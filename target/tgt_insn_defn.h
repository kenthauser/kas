#ifndef KAS_Z80_Z80_INSN_TYPES_H
#define KAS_Z80_Z80_INSN_TYPES_H

///////////////////////////////////////////////////////////////////////////////////////
//
// instruction data structures: stored & runtime
//
///////////////////////////////////////////////////////////////////////////////////////

//#include "z80_insn_validate.h"
//#include "z80_formats_type.h"
#include "target/tgt_validate.h"
#include "target/tgt_format.h"

#include "m68k/m68k_size_lwb.h"

namespace kas::tgt::opc
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

template <typename T>
uint8_t constexpr code_to_words(std::size_t value, uint8_t N = 1)
{
    using limit = std::numeric_limits<T>;
    if (value > limit::max())
        return code_to_words<T>(value >> limit::digits, N + 1);
    return N;
}

// XXX per-instruction constexpr definition
template <typename MCODE_T>
struct tgt_insn_defn
{
    // import definitions from MCODE_T
    using mcode_t      = MCODE_T;
    using mcode_size_t = typename mcode_t::mcode_size_t;
    using fmt_t        = typename mcode_t::fmt_t;
    using val_t        = typename mcode_t::val_t;
    using val_c_t      = typename mcode_t::val_c_t;
    using adder_t      = typename mcode_t::adder_t;
    using name_idx_t   = typename mcode_t::name_idx_t;
    using fmt_idx_t    = typename mcode_t::fmt_idx_t;
    using val_c_idx_t  = typename mcode_t::val_c_idx_t;
    static constexpr auto MAX_ARGS = mcode_t::MAX_ARGS;

    // NAME the `defn` INDEXES
    // NB: this is only for reference. The list
    // is passed as a whole to the ctor, so any changes
    // in defn must also be reflected there.
    static constexpr auto DEFN_IDX_SZ   = 0;
    static constexpr auto DEFN_IDX_NAME = 1;
    static constexpr auto DEFN_IDX_INFO = 2;
    static constexpr auto DEFN_IDX_FMT  = 3;
    static constexpr auto DEFN_IDX_VAL  = 4;
    
    using VAL_SEQ   = std::make_index_sequence<DEFN_IDX_VAL + MAX_ARGS>;

    using NAME_LIST = list<int_<DEFN_IDX_NAME>>;
    using SIZE_LIST = list<int_<DEFN_IDX_SZ>>;
    using FMT_LIST  = list<int_<DEFN_IDX_FMT>>;
    using VAL_LIST  = drop_c<IS_as_list<VAL_SEQ>, DEFN_IDX_VAL>;

    using XLATE_LIST = list<list<const char * , NAME_LIST>
                          , list<const m68k::opc::m68k_insn_size, SIZE_LIST>
                          , list<const fmt_t *, FMT_LIST, quote<VT_CTOR>>
                          , list<const val_t *, VAL_LIST, quote<VT_CTOR>>

                          // val_combos: don't have the `sym_parser` init combo
                          , list<void, VAL_LIST, void, list<>, quote<list>>
                          >;

    using ADDER  = adder_t;

    template <typename NAME, typename SZ, typename FMT, typename...VALs, typename VAL_C,
              typename S, typename N, typename OP, typename...X>
    constexpr tgt_insn_defn(list<list<list<NAME>, list<SZ>, list<FMT>
                                     , list<VALs...>, list<VAL_C>>
                                , list<S, N, OP, X...>>)
            : name_index  { NAME::value  + 1   }
            , sz_index    { SZ::value    + 1   }
            , fmt_index   { FMT::value   + 1   }
            , val_c_index { VAL_C::value + 1   }
            , code        { OP::opcode::value  }
            , code_words  { code_to_words<mcode_size_t>(OP::opcode::value) }
            , size_fn     { typename OP::size_fn{} }
            //, tst         { OP::tst::value     }
            {}
            
    // `fmt_t` is abstract class. access via pminters to instances
    static inline const char * const *names_base;
    static inline const m68k::opc::m68k_insn_size *sizes_base;
    static inline const fmt_t *const *fmts_base;
    static inline val_c_t      const *val_c_base;

    void print(std::ostream& os) const
    {
        os << std::dec;
        os << "tgt_insn_defn: name_idx = " << +name_index;
        os << " val_c_index = " << +val_c_index;
        os << " fmt_index = " << +fmt_index;
        os << std::hex;
        os << " code = " << +code;
        os << std::dec;
        os << std::endl;
        os << " -> name = \"" << name();
        os << "\" vals: " << vals();
    }
    
    // alt gives alternate suffix, if available.
    // arch gives mit/moto alternate, if runtime configured
    // `name(sz)` gives canonical name
    auto name() const
    {
        return names_base[name_index - 1];
    }

    auto& fmt()  const  { return *fmts_base [fmt_index   - 1]; }
    auto& vals() const  { return  val_c_base[val_c_index - 1]; }

    // XXX
    uint32_t code;          // actual binary code
    uint16_t tst {};           // hw test
    uint8_t  code_words;    // zero-based
    m68k::opc::insn_lwb size_fn;

    // override sizes in `MCODE_T`
    name_idx_t  name_index;    //  ? bits
    val_c_idx_t val_c_index;   //  ? bits
    fmt_idx_t   fmt_index;     //  ? bits
    uint8_t     sz_index;
};

}
#endif

