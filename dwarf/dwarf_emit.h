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


#include "dwarf_opc_impl.h"

// includes for insertion into container
#include "kas_core/insn_container.h"
#include "kas_core/insn_container_data.h"

// includes for `opcodes`
#include "kas_core/opc_fixed.h"
#include "kas_core/opc_leb.h"

// includes for `kas` utilities
#include "kas/kas_string.h"
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

struct emit_insn
{
    using insn_inserter_t = core::insn_inserter_t;


    emit_insn(insn_inserter_t& inserter) : inserter(inserter) {}

    // destructor: emit current in-progress insn
    ~emit_insn() { flush(); }

    // emit dwarf "named" insn as `core::core_insn`
    template <typename DEFN, typename Arg
            , typename OP = typename  DEFN::op>
    void operator()(DEFN&& d, Arg&& arg)
    {
//#define TRACE_DWARF_EMIT
#ifdef TRACE_DWARF_EMIT
        if constexpr (std::is_integral_v<std::remove_reference_t<Arg>>)
            std::cout << " " << DEFN() << ": arg = " << +arg << std::endl;
        else
            std::cout << " " << DEFN() << ": arg = " << arg << std::endl;
#endif
        // if currently inserting fixed, verify same `OP` type
        if (!insn_p || !op_p->is_same(d))
            set_opcode(d);

        do_fixed_emit(std::forward<Arg>(arg));
    }

    // emit other ops (eg labels, align)
    template <typename OPCODE, typename...Ts,
            typename = std::enable_if_t<std::is_base_of_v<core::opcode, OPCODE>>>
    void operator()(OPCODE const& op, Ts&&...args)
    {
#ifdef TRACE_DWARF_EMIT
        std::cout << " OPCODE:" << op.name();
        auto tpl = std::make_tuple(args...);
        if constexpr (sizeof...(Ts) == 1)
            std::cout << ": arg = " << +std::get<0>(tpl) << std::endl;
        else
            std::cout << ": arg count = " << sizeof...(Ts) << std::endl;
#endif
        flush();                  // emit pending insn
        *inserter++ = { op, std::forward<Ts>(args)...};
    }

    // emit symbol as label
    void operator()(core::symbol_ref const& sym)
    {
#ifdef TRACE_DWARF_EMIT
        std::cout << " label: arg = " << sym << std::endl;
#endif
        flush();                  // emit pending insn
        *inserter++ = { core::opc::opc_label{}, sym };
    }

    // emit addr as label
    void operator()(core::core_addr_t const& addr)
    {
#ifdef TRACE_DWARF_EMIT
        std::cout << " label: arg = " << addr << std::endl;
#endif
        flush();                  // emit pending insn
        *inserter++ = { core::opc::opc_label{}, addr };
    }

    auto& get_dot(int which = core::core_addr_t::DOT_CUR)
    {
#ifdef TRACE_DWARF_EMIT
        const char *msg = 
                (which == core::core_addr_t::DOT_NEXT) ? "NEXT" : "CURRENT";
        std::cout << " GET_DOT: " << msg << std::endl;
#endif
        // if `dot` requested, must flush pending data before evaluating
        // NB: `dot_after` implemented in `insn_container`, which doesn't
        // understand `pending_insns`
        flush(); 
        if (which == core::core_addr_t::DOT_NEXT)
            pending_dot_after = true;   // dot after next insn
        return core::core_addr_t::get_dot(which);
    }

    
private:
    // emit arg using `core_data` derived type
    template <typename Arg>
    void do_fixed_emit(Arg&& arg)
    {
        insn_p->data.size += (*op_p)(di_p, std::forward<Arg>(arg));

        // limit size of INSN
        static constexpr auto MAX_INSN_SIZE = 1024;

        if (insn_p->data.size() >= MAX_INSN_SIZE)
            flush();

        // part of `get_dot(DOT_NEXT)` implementation.
        if (pending_dot_after)
        {
#ifdef TRACE_DWARF_EMIT
            std::cout << "do_fixed_emit: have pending dot" << std::endl;
#endif
            pending_dot_after = false;
            flush();
        }
    }

    // emit insn if partial one is pending
    void flush()
    {
#ifdef TRACE_DWARF_EMIT
        if (insn_p)
        {
            std::cout << "emit_insn::flush: raw : ";
            insn_p->raw(std::cout);
            std::cout << std::endl;
            std::cout << "emit_insn::flush: fmt : ";
            insn_p->fmt(std::cout);
            std::cout << std::endl;
        }
#endif
        // insert & destroy `insn`, destroy `data_inserter`
        if (insn_p)
        {
            op_p->delete_inserter(di_p);
            *inserter++ = std::move(*insn_p);
            insn_p = {};
            op_p   = {};
            di_p   = {};
        }
    }

    void set_opcode(dwarf_emit_fixed const& d)
    {
        // flush current insn if required
        if (insn_p) flush();

        // start new instruction 
        // create new `insn` for current opcode
        // for reference, inspect stmt_t::operator() method
        // in "parser/parser_variant.h"
        
        // construct insn from `op`
        insn_p = std::make_unique<core::core_insn>();
        insn_p->opc_index = d.get_opc_index();
        
        // create a fixed data inserter appropriate for `op`
        di_p = d.make_inserter(insn_p->data);   // delete in flush
        
        // save pointer to `op` routines
        op_p = d.get_p();
    }

private:
    // inserter to emit insn into container
    insn_inserter_t& inserter;
    
    std::unique_ptr<core::core_insn> insn_p;// current insn
    dwarf_emit_fixed const * op_p {};       // fixed_insn operations
    void                   * di_p {};       // pointer to data_inserter
    bool pending_dot_after{};               // dot-after requested & pending
};
#undef TRACE_DWARF_EMIT

}

#endif
