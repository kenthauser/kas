#ifndef KAS_M68K_M68K_INSN_TYPES_H
#define KAS_M68K_M68K_INSN_TYPES_H

// m68k instruction data structures: stored & runtime
//
///////////////////////////////////////////////////////////////////////////////////////

#include "m68k_insn_validate.h"
#include "m68k_formats_type.h"
#include "m68k_size_lwb.h"

namespace kas::m68k::opc
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

struct m68k_insn_adder;

// per-instruction constexpr definition
struct m68k_insn_defn
{
    // import MAX_ARGS defn
    static constexpr auto MAX_ARGS = m68k_insn_t::MAX_ARGS;

    static constexpr auto VALIDATOR_BASE = 4;
    using VAL_SEQ   = std::make_index_sequence<VALIDATOR_BASE + MAX_ARGS>;

    using NAME_LIST = list<int_<1>>;
    using SIZE_LIST = list<int_<0>>;
    using FMT_LIST  = list<int_<3>>;
    using VAL_LIST  = drop_c<IS_as_list<VAL_SEQ>, VALIDATOR_BASE>;

    using XLATE_LIST = list<list<const char *           , NAME_LIST>
                          , list<m68k_insn_size         , SIZE_LIST>
                          , list<const m68k_opcode_fmt *, FMT_LIST, quote<VT_CTOR>>
                          , list<const m68k_validate *  , VAL_LIST, quote<VT_CTOR>>

                          // val_combos: don't have the `sym_parser` init combo
                          , list<void, VAL_LIST, void, list<>, quote<list>>
                          >;

    // hook for `parser::sym_parser_t`
    using ADDER  = m68k_insn_adder;

    template <typename NAME, typename SZ, typename FMT, typename...VALs, typename VAL_C,
              typename S, typename N, typename OP, typename...D>
    constexpr m68k_insn_defn(list<list<list<NAME>, list<SZ>, list<FMT>
                                     , list<VALs...>, list<VAL_C>>
                                , list<S, N, OP, D...>>)
            : name_index  { NAME::value  + 1   }
            , sz_index    { SZ::value    + 1   }
            , fmt_index   { FMT::value   + 1   }
            , val_c_index { VAL_C::value + 1   }
            , opcode      { OP::opcode::value  }
            , lwb_fn      { typename OP::size_fn{} }
            //, tst         { OP::tst::value     }
            {}
            
    static inline const char *            const *names_base;
    static inline const m68k_insn_size          *sizes_base;
    static inline const m68k_opcode_fmt * const *fmts_base;
    static inline const m68k_validate_args      *val_c_base;
    
    // alt gives alternate suffix, if available.
    // arch gives mit/moto alternate, if runtime configured
    // `name(sz)` gives canonical name
    auto name() const
    {
        return names_base[name_index - 1];
    }

    auto& fmt()   const  { return *fmts_base [fmt_index   - 1]; }
    auto& val_c() const  { return  val_c_base[val_c_index - 1]; }

    uint32_t opcode;        // actual binary code
    uint16_t name_index;    // 11 bits
    uint16_t val_c_index;   //  8 bits
    uint8_t  fmt_index;     //  7 bits
    uint8_t  sz_index;      //  5 bits
    insn_lwb lwb_fn;        //  8 bits
};

// instruction per-size run-time object
// NB: not allocated if info->hw_tst fails, unless no
// other insn with name allocated...

struct m68k_opcode_t
{
    using fmt_t     = m68k_opcode_fmt;
    using err_msg_t = error_msg;
    static constexpr auto MAX_ARGS = 6;     // coldfire emac requires lots of args

    // allocate instances in `std::deque`
    using obstack_t = std::deque<m68k_opcode_t>;

    m68k_opcode_t(uint16_t index
                , uint16_t defn_index, m68k_insn_defn const *p
                , uint8_t sz, bool single_sz)
        : index(index), defn_index(defn_index), insn_sz(sz)
    {
        // what to add for "LWB" indication
        lwb_code = single_sz ? 0 : p->lwb_fn.lwb_code(sz);

        // test opcode word length
        opc_long = p->opcode > 0xffff;
        
        // must shift code iff long insn & code destined for word 0
        lwb_long = lwb_code && opc_long && (p->lwb_fn.sz_word == 0);
    }

    // template so don't need to include `tgt_insn.h` & derived type
    // validate arg count & arg_modes are supported on run-time target
    template <typename ARGS_T>
    const char *validate_args (ARGS_T& args, std::ostream *trace = {}) const;

    template <typename ARGS_T>
    fits_result size(ARGS_T& args, op_size_t&, expr_fits const&, std::ostream *trace = {}) const;

    template <typename ARGS_T>
    void emit(core::emit_base&, m68k_ext_size_t *, ARGS_T&&, core::core_expr_dot const&) const;

    auto  sz()    const { return static_cast<m68k_size_t>(insn_sz); }
    auto& defn()  const { return defns_base[defn_index]; }
    auto& fmt()   const { return defn().fmt(); }
    //auto& val_c() const { return defn().val_c();         }

    uint32_t code() const
    {
        auto shift = lwb_long ? 16 : 0;
        if (lwb_long)
            return defn().opcode | (lwb_code << 16);
        return defn().opcode | (lwb_code << shift);
    }

    static inline const obstack_t      *index_base;
    static inline const m68k_insn_defn *defns_base;

    // retrieve instance from index
    static auto& get(uint16_t idx) { return (*index_base)[idx]; }
    
    uint16_t index;         // -> access this instance (zero-based)
    uint16_t defn_index;    // -> access name, fmt, validator (zero-based)
    uint16_t lwb_code;      // -> add to opcode to encode size
    uint16_t insn_sz  : 3;  // 3 bits (displacement size);
    uint16_t opc_long : 1;  // 32-bit opcode
    uint16_t lwb_long : 1;  // must shift "lwb" code
   
    // XXX can copy validators index to runtime to complete 64-bit word
};
}
#endif

