#ifndef KAS_UTILITY_STRING_MPL_H
#define KAS_UTILITY_STRING_MPL_H

#include "kas/kas_string.h"

namespace kas
 {
    namespace detail
    {
        template <unsigned, char...> struct i2s_impl;

        // always output at least a single zero
        template <char N, char...NUM>
        struct i2s_impl<0, N, NUM...>
            : string::kas_string<char, N, NUM...> {};
        
        // recurse till most-significant digit is zero
        template <unsigned N, char...NUM>
        struct i2s_impl
             : i2s_impl<(N/10), (N%10)+'0', NUM...> {};

    }

    template <unsigned N>
    using i2s = meta::_t<detail::i2s_impl<N>>;

    template <unsigned N
            , typename PFX = string::kas_string<>
            , typename SFX = string::kas_string<>
            >
    using i2list = string::str_cat<PFX, i2s<N>, SFX>;
}

#endif
