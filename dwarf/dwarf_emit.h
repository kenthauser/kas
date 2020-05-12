#ifndef KAS_DWARF_DWARF_EMIT_H
#define KAS_DWARF_DWARF_EMIT_H

// Generate a stream of instructions containing DWARF data
//
// Dwarf programs are a long string of short, fixed value data 
// instructions. This data is packaged into `core_insn` instructions
// using the KAS `fixed` data opcodes used for the `.byte`, `.word`, etc
// pseudo-ops and stored in an instruction container.
//
// Since all of the `fixed` data opcodes can accept an unlimited number
// of arguments, several consecutive DWARF opcodes of the same type are
// merged into a single `fixed` data opcode. This is much more efficient
// for the backend code.
//
// The `dwarf::emit_insn` type generates an `core_insn` corresponding to the
// dwarf opcode & then emulates the driver functions the fixed-code base
// type `core::opc::opc_data`. The type `dwarf::emit_insn` can also generate
// an `core::opc::opc_label` insn & others as required.
//
// The `dwarf::emit_insn` type makes use of a number of low-level data
// structures and might be best viewed as a black box...


#include "kas/kas_string.h"
#include "kas_core/opc_fixed.h"
#include "kas_core/opc_leb.h"
#include "kas_core/core_fixed_inserter.h"
#include "utility/print_type_name.h"

namespace kas::dwarf
{

// retrieve `value_t` member type, if present (default: void)
// `core::opc::opc_data` types have this member type
template <typename OP, typename = void>
struct get_value_impl : meta::id<void> {};

template <typename OP>
struct get_value_impl<OP, std::void_t<typename OP::value_t>>
        : meta::id <typename OP::value_t> {};

template <typename OP>
using get_value_t = meta::_t<get_value_impl<OP>>;

template <typename INSN_INSERTER>
struct emit_insn
{
    emit_insn(INSN_INSERTER& inserter) : inserter(inserter) {}

    // destructor: emit current in-progress insn
    ~emit_insn() { do_emit(); }

    // emit dwarf "named" insn as `core::core_insn`
    template <typename DEFN, typename Arg
            , typename OP = typename  DEFN::op>
    void operator()(DEFN, Arg&& arg)
    {
        //std::cout << " " << std::string(DEFN()) << ": arg = " << expr_t(arg) << std::endl;
        do_fixed_emit<OP>(std::forward<Arg>(arg));
    }

    // emit other ops (eg labels, align)
    template <typename OPCODE, typename...Ts,
            typename = std::enable_if_t<std::is_base_of_v<core::opcode, OPCODE>>>
    void operator()(OPCODE const& op, Ts&&...args)
    {
        do_emit();                  // emit pending insn
        *inserter++ = { op, std::forward<Ts>(args)...};
    }

    // emit symbol as label
    void operator()(core::symbol_ref const& sym)
    {
        do_emit();                  // emit pending insn
        *inserter++ = { core::opc::opc_label{}, sym };
    }

    auto& get_dot(int which = core::core_addr_t::DOT_CUR)
    {
        // if `dot` requested, must flush pending data before evaluating
        do_emit();                      // next opcode may not match
        if (which == core::core_addr_t::DOT_NEXT)
            pending_dot_after = true;   // dot after next insn
        return core::core_addr_t::get_dot(which);
    }
    
private:
    // fixed emit ops require a data inserter. Declare type
    template <typename value_t>
    using di_t = core::opc::detail::fixed_inserter_t<value_t>;

    // emit arg using `core_data` derived type
    template <typename OP, typename Arg>
    void do_fixed_emit(Arg&& arg)
    {
        set_opcode<OP>();                       // make `OP` current insn type
        
        using value_t = get_value_t<OP>;        // defined for `core_data` types
        auto& di = this->di.get(value_t());     // retrieve data inserter

        // NB: normal OP::proc_one methods take `kas_tokens`.
        // Overloads for dwarf types have been impemented in relevent `OP`
        if constexpr (std::is_integral_v<Arg>)
            insn.data.size += OP::proc_one(di, arg);
        else if constexpr (std::is_same_v<std::decay_t<Arg>, const char *>)
            insn.data.size += OP::proc_one(di, arg);
        else
            insn.data.size += OP::proc_one(di, expr_t(std::forward<Arg>(arg)));

        // part of `get_dot(DOT_NEXT)` implementation.
        if (pending_dot_after)
        {
            pending_dot_after = false;
            do_emit();
        }
    }

    // emit insn if partial one is pending
    void do_emit()
    {
        if (insn.opc_index != 0)
        {
            std::cout << "do_emit: raw : ";
            insn.raw(std::cout);
            std::cout << std::endl;
            std::cout << "do_emit: fmt : ";
            insn.fmt(std::cout);
            std::cout << std::endl;
        }

        if (insn.opc_index != 0)
            *inserter++ = std::move(insn);
        insn.opc_index    = 0;      // insn is empty
    }

    template <typename OP>
    void set_opcode()
    {
        // fixed data opcodes all have a `value_t` member type
        using value_t = get_value_t<OP>;
        
        // don't allow excessive data in insns...
        if (insn.data.size() > 1024)
            do_emit();          // that's big enough...

        OP op;                  // default construct opcode

        // if different `OP`, need to emit old insn & start new one
        if (op.index() != insn.opc_index)
        {
            do_emit();          // insert previous insn into container

            // create new `insn` for current opcode
            // for reference, inspect stmt_t::operator() method
            // in "parser/parser_variant.h"
            
            // NB: insn & data_inserter (& members) don't have dtors. 
            // NB: no need to destruct before re-construct...
            static_assert(std::is_trivially_destructible_v<decltype(insn)>);
            static_assert(std::is_trivially_destructible_v<di_t<value_t>>);

            // reconstruct insn as `op`
            new(&insn) decltype(insn)();
            insn.opc_index = op.index();

            // reconstruct the `data_inserter` needed to emulate `core_data`
            new(&di.get(value_t())) di_t<value_t>(insn.data.di(), insn.data.fixed);
        }
    }

private:
    // data inserter part of the `opc_data` emulation
    union data_inserter
    {
        // needs default ctor
        data_inserter() {}

        template <typename T>
        auto& get(T)
        {
            if constexpr (std::is_same_v<T, char>)
                return di_8;
            else if constexpr (std::is_same_v<T, std::uint8_t>)
                return di_u8;
            else if constexpr (std::is_same_v<T, std::uint16_t>)
                return di_u16;
            else if constexpr (std::is_same_v<T, std::uint32_t>)
                return di_u32;
            else if constexpr (std::is_same_v<T, std::uint64_t>)
                return di_u64;
            else
                // pick templated assertion that will fail...
                static_assert(std::is_same_v<T, char>,
                              "invalid data_inserter type");
        }
        
        // data inserters
        di_t<char>          di_8;
        di_t<std::uint8_t > di_u8;
        di_t<std::uint16_t> di_u16;
        di_t<std::uint32_t> di_u32;
        di_t<std::uint64_t> di_u64;
    } di;
    
    // inserter to emit insn into container
    INSN_INSERTER& inserter;
    core::core_insn insn;       // current insn
    bool pending_dot_after{};   // dot-after requested & pending

};

}

#endif
