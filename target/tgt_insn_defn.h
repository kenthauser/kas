#ifndef KAS_TARGET_TGT_DEFN_H
#define KAS_TARGET_TGT_DEFN_H

///////////////////////////////////////////////////////////////////////////////////////
//
// instruction data structures: stored & runtime
//
///////////////////////////////////////////////////////////////////////////////////////

#include "tgt_defn_trait.h"
#include "tgt_insn_adder.h"
#include "tgt_validate.h"
#include "tgt_format.h"

#include "parser/init_from_list.h"  // get VT_CTOR

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

// Special constructor for virtual types (virtual members or virtual bases)
//
// Since C++ doesn't allow a virtual base-type to hold a derived type, 
// construct the derived type as a static, and use a pointer to this
// static instance as `defn` value
//
// Two types of "virtual" types are instantiated:
//  - validators
//  - formatters
//
// The validators are passed as a `meta::list` of two or more items:
//      the first type is the "NAME" of the validator (for debugging)
//      the second type is base class of the validator
//      subsequent arguments are `std::integral_constant` wrapped values for base class
//
// The formatters are identified by a single `type` to be instantiated.
//      However, if the `type` is `void`, the `default` formatter type
//      is retrieved from the `base-type` & used.

template <typename T>
uint8_t constexpr code_to_words(std::size_t value, uint8_t N = 1)
{
    using limit = std::numeric_limits<T>;
    if (value > limit::max())
        return code_to_words<T>(value >> limit::digits, N + 1);
    return N;
}

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
    using defn_sizes_t = typename mcode_t::defn_sizes_t;

    using fmt_default  = typename mcode_t::fmt_default;     // default type (or void)

    // import size definitions from MCODE_T
    using name_idx_t   = typename mcode_t::name_idx_t;
    using fmt_idx_t    = typename mcode_t::fmt_idx_t;
    using val_c_idx_t  = typename mcode_t::val_c_idx_t;
    static constexpr auto MAX_ARGS = mcode_t::MAX_ARGS;

    // define LISTS for `adder::XLATE_LIST`
    using NAME_LIST = list<int_<traits::DEFN_IDX_NAME>>;
    using SIZE_LIST = list<int_<traits::DEFN_IDX_SZ>, int_<traits::DEFN_IDX_INFO>>;  
    using FMT_LIST  = list<int_<traits::DEFN_IDX_FMT>>;
    
    // create list with integer sequence <IDX_VAL...(IDX_VAL+MAX_ARGS)>
    using VAL_LIST  = drop_c<IS_as_list<std::make_index_sequence<traits::DEFN_IDX_VAL + MAX_ARGS>>
                           , traits::DEFN_IDX_VAL
                           >;

    // wrap `fmt_default` in `meta::list` if specified
    using fmt_dflt_list = if_<std::is_void<fmt_default>, list<>, list<fmt_default>>;

    // `ADDER` & `XLATE_LIST` are picked up by `sym_parser_t`
    using ADDER      = adder_t;
    using XLATE_LIST = list<list<const char *      , NAME_LIST>
                          , list<const defn_sizes_t, SIZE_LIST, void, list<>, quote<list>>
                          , list<const fmt_t *     , FMT_LIST, parser::VT_CTOR, fmt_dflt_list>
                          , list<const val_t *     , VAL_LIST, parser::VT_CTOR>

                          // val_combos: don't have the `sym_parser` generate defn
                          , list<void              , VAL_LIST, void, list<>, quote<list>>
                          >;

    // declare indexes into XLATE list (used by `adder`)
    static constexpr auto XLT_IDX_NAME = 0;
    static constexpr auto XLT_IDX_SIZE = 1;
    static constexpr auto XLT_IDX_FMT  = 2;
    static constexpr auto XLT_IDX_VAL  = 3;
    static constexpr auto XLT_IDX_VALC = 4;

    // CTOR: passed list<defn_list, xlt_list>
    template <typename NAME, typename SZ, typename FMT, typename...VALs, typename VAL_C,
              typename S, typename N, typename CODE, typename TST, typename...X>
    constexpr tgt_insn_defn(list<list<list<NAME>, list<SZ>, list<FMT>
                                     , list<VALs...>, list<VAL_C>>
                                , list<S, N, CODE, TST, X...>>)
            : name_index  { NAME::value  + 1   }
            , sz_index    { SZ::value          }
            , fmt_index   { FMT::value   + 1   }
            , val_c_index { VAL_C::value + 1   }
            , code        { CODE::value        }
            , code_words  { code_to_words<mcode_size_t>(CODE::value) }
            //, tst         { TST::value     }
            {}
            
    static inline const char * const *names_base;
    static inline const defn_sizes_t *sizes_base;
    static inline const fmt_t *const *fmts_base;
    static inline val_c_t      const *val_c_base;

    auto  name() const { return  names_base[name_index - 1];  }
    auto& fmt()  const { return *fmts_base [fmt_index   - 1]; }
    auto& vals() const { return  val_c_base[val_c_index - 1]; }

    // don't inline diag function
    void print(std::ostream& os) const;
    
    // (contexpr) instance data 
    uint32_t code;          // actual binary code (base value)
    uint16_t tst {};        // hw test

    // override sizes in `MCODE_T`
    name_idx_t  name_index;    
    val_c_idx_t val_c_index;   
    fmt_idx_t   fmt_index;    
    uint8_t     sz_index;       
    uint8_t     code_words;     // zero-based count of words
};

// don't inline support routines
template <typename MCODE_T>
void tgt_insn_defn<MCODE_T>::print(std::ostream& os) const
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
    
}
#endif

