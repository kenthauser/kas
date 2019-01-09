#ifndef KAS_Z80_Z80_INSN_TYPES_H
#define KAS_Z80_Z80_INSN_TYPES_H

///////////////////////////////////////////////////////////////////////////////////////
//
// z80 instruction data structures: stored & runtime
//
///////////////////////////////////////////////////////////////////////////////////////

#include "z80_insn_validate.h"
#include "z80_formats_type.h"

namespace kas::z80::opc
{
using namespace meta;
//using namespace hw;

//// convert IntegerSequence to meta::list
// XXX move to common location...
namespace detail
{
    template <typename Seq> struct IS_as_list_impl;

    template <typename T, T...Ts>
    struct IS_as_list_impl<std::integer_sequence<T, Ts...>>
    {
        using type = meta::list<std::integral_constant<T, Ts>...>;
    };
}

template <typename Seq>
using IS_as_list  = meta::_t<detail::IS_as_list_impl<Seq>>;

// Special CTOR for virtual types: return pointer to instance
template <typename T, typename = void>
struct vt_ctor_impl : vt_ctor_impl<list<void, T>> {};

template <typename NAME, typename T, typename...Ts>
struct vt_ctor_impl<list<NAME, T, Ts...>, void>
{
    using type = vt_ctor_impl;
    static const inline T data{Ts::value...};
    static constexpr auto value = &data;
};

template <typename...>
struct VT_CTOR
{
    using type = VT_CTOR;

    template <typename DEFN>
    using invoke = _t<vt_ctor_impl<DEFN>>;
};

////

struct z80_insn_adder;

// per-instruction constexpr definition
struct z80_insn_defn
{
    // import MAX_ARGS defn
    static constexpr auto MAX_ARGS = z80_insn_t::MAX_ARGS;

    static constexpr auto VALIDATOR_BASE = 3;
    using VAL_SEQ   = std::make_index_sequence<VALIDATOR_BASE + MAX_ARGS>;

    using NAME_LIST = list<int_<0>>;
    using FMT_LIST  = list<int_<2>>;
    using VAL_LIST  = drop_c<IS_as_list<VAL_SEQ>, VALIDATOR_BASE>;

    using XLATE_LIST = list<list<const char *          , NAME_LIST>
                          , list<const z80_opcode_fmt *, FMT_LIST, quote<VT_CTOR>>
                          , list<const z80_validate *  , VAL_LIST, quote<VT_CTOR>>

                          // val_combos: don't have the `sym_parser` init combo
                          , list<void, VAL_LIST, void, list<>, quote<list>>
                          >;

    // hook for `parser::sym_parser_t`
    using ADDER  = z80_insn_adder;

    template <typename NAME, typename FMT, typename...VALs, typename VAL_C,
              typename N, typename OP, typename...D>
    constexpr z80_insn_defn(list<list<list<NAME>, list<FMT>
                                     , list<VALs...>, list<VAL_C>>
                                , list<N, OP, D...>>)
            : name_index  { NAME::value  + 1   }
            , fmt_index   { FMT::value   + 1   }
            , val_c_index { VAL_C::value + 1   }
            , opcode      { OP::value  }
            //, tst         { OP::tst::value     }
            {}
            
    static inline const char *            const *names_base;
    static inline const z80_opcode_fmt *  const *fmts_base;
    static inline const z80_validate_args       *val_c_base;
    
    // alt gives alternate suffix, if available.
    // arch gives mit/moto alternate, if runtime configured
    // `name(sz)` gives canonical name
    auto name() const
    {
        return names_base[name_index - 1];
    }

    auto& fmt()   const  { return *fmts_base [fmt_index   - 1]; }
    auto& val_c() const  { return  val_c_base[val_c_index - 1]; }

    uint16_t opcode;        // actual binary code
    uint8_t  name_index;    //  ? bits
    uint8_t  val_c_index;   //  ? bits
    uint8_t  fmt_index;     //  ? bits
};

// instruction per-size run-time object
// NB: not allocated if info->hw_tst fails, unless no
// other insn with name allocated...

struct z80_opcode_t
{
    using fmt_t     = z80_opcode_fmt;
    using err_msg_t = error_msg;
    static constexpr auto MAX_ARGS = 2;
    using mcode_size_t = uint8_t;

    // allocate instances in `std::deque`
    using obstack_t = std::deque<z80_opcode_t>;

    z80_opcode_t(uint16_t index, uint16_t defn_index, z80_insn_defn const *p)
        : index(index), defn_index(defn_index)
    {
        // test opcode word length
        opc_long = p->opcode > 0xff;
    }

    // make template so don't need to include `tgt_insn.h` & derived type
    // validate arg count & arg_modes are supported on run-time target
    template <typename ARGS_T>
    const char * validate_args (ARGS_T& args, std::ostream *trace = {}) const;

    template <typename ARGS_T>
    fits_result size(ARGS_T& args, op_size_t& size, expr_fits const&, std::ostream *trace = {}) const;

    template <typename ARGS_T>
    void emit(core::emit_base&, uint16_t *, ARGS_T&&, core::core_expr_dot const * = {}) const;

    auto& defn()  const { return defns_base[defn_index]; }
    auto& fmt()   const { return defn().fmt(); }
    auto& vals()  const { return defn().val_c(); }
    auto  sz()    const { return 0; }

    uint32_t code() const
    {
        return defn().opcode;
    }

    static inline const obstack_t      *index_base;
    static inline const z80_insn_defn *defns_base;

    // retrieve instance from index
    static auto& get(uint16_t idx) { return (*index_base)[idx]; }
    
    uint16_t index;         // -> access this instance (zero-based)
    uint16_t defn_index;    // -> access name, fmt, validator (zero-based)
    uint16_t opc_long : 1;  // 32-bit opcode
};
}
#endif

