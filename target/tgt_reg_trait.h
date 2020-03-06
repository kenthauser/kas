#ifndef KAS_TARGET_TGT_REG_TRAIT_H
#define KAS_TARGET_TGT_REG_TRAIT_H

////////////////////////////////////////////////////////////////////////////
//
// Define a MPL function to create a sequence of registers (eg: a0->a15)
//
// called: make_reg_seq<CALLABLE, NAME_BASE, REG_COUNT, BASE_COUNT = 0>
//
// CALLABLE invoked for each register with list<NAME_N, N>
//
// return list<> of CALLABLEs
//
////////////////////////////////////////////////////////////////////////////

#include "kas/kas_string.h"     // need KAS_STRING: `i2list`
#include <meta/meta.hpp>

namespace kas::tgt::reg_defn
{
using namespace meta;

// cut down on syntax noise
template <typename NAME, unsigned REG, unsigned NUM = 0, typename TST = void>
using reg = list<NAME
               , std::integral_constant<unsigned, REG>
               , std::integral_constant<unsigned, NUM>
               , std::conditional_t<std::is_void_v<TST>, int_<0>, TST>
               >;

// meta callable used by `make_reg_seq` to generate a `reg` 
// invoked with types "STR(nameN), N"
template <int KIND, typename TST = void>
struct reg_seq
{
    template <typename NAME, typename NUM>
    using invoke = reg<NAME, KIND, NUM::value, TST>;
};

// implement `make_reg_seq`
namespace detail
{
    using namespace meta;

    template <typename NAME>
    struct reg_n
    {
        // returns list<IS<nameN>, N>
        template <typename N>
        using invoke = list<i2list<N::value, NAME>, N>;
    };

    // called: `...<reg_seq<RC_DATA>, IS<'d'>, 8>`
    template <typename FN, typename NAME, unsigned COUNT, unsigned BASE>
    struct make_reg_seq : transform<
                  transform<
                        as_list<integer_range<unsigned, BASE, BASE+COUNT>>
                      , reg_n<NAME>
                      >
                , uncurry<FN>
                > {};

}

// generate a sequence of register names from base class, base name, and count
template <typename FN
        , typename NAME
        , unsigned COUNT
        , unsigned BASE = 0
        >
using make_reg_seq = _t<detail::make_reg_seq<FN, NAME, COUNT, BASE>>;
}

#endif
