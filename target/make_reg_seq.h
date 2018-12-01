#ifndef KAS_TARGET_MAKE_REG_SEQ_H
#define KAS_TARGET_MAKE_REG_SEQ_H

#include "utility/string_mpl.h"

// called: make_reg_seq<CALLABLE, NAME_BASE, REG_COUNT, BASE_COUNT = 0>
//
// CALLABLE invoked for each register with list<NAME_N, N>
//
// return list<> of CALLABLEs

namespace kas
{

     namespace detail
     {
        using namespace meta;

        template <typename NAME>
        struct reg_n
        {
            // returns meta::list<IS<nameN>, N>
            template <typename N>
            using invoke = list<i2list<N::value, NAME>, N>;
        };

        // called: ``...<reg_seq<RC_DATA>, IS<'d'>, 8>``
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
    using make_reg_seq = meta::_t<detail::make_reg_seq<FN, NAME, COUNT, BASE>>;
}

#endif
