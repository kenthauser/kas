#ifndef KAS_ARM_ARM_EABI_DEFNS_H
#define KAS_ARM_ARM_EABI_DEFNS_H

// Declare ABI attribute definitions for the ARM Architecture
//
// Source: "Addenda to, and Errata in, the ABI for ARM Architecture"
// Updated to: 2021Q1
// URL: github.com/ARM-software/abi-aa/addenda32

#include "kas/kas_string.h"
#include <meta/meta.hpp>

namespace kas::arm::eabi
{
using namespace meta;
#define STR(_) KAS_STRING(#_)

template <typename NAME>
using ARCH = kas::str_cat<STR(arm), NAME>;

template <typename NAME, unsigned value, bool deprecated = false>
using arch_trait = list<ARCH<NAME>, int_<value>, bool_<deprecated>>;

using arm_arch = list<
      arch_trait<STR(v4)       , 1>
    , arch_trait<STR(v4t)      , 2>
    , arch_trait<STR(v5t)      , 3>
    , arch_trait<STR(v5te)     , 4>
    , arch_trait<STR(v5tej)    , 5>
    , arch_trait<STR(v6)       , 6>
    , arch_trait<STR(v6kz)     , 7>
    , arch_trait<STR(v6t2)     , 8>
    , arch_trait<STR(v6k)      , 9>
    , arch_trait<STR(v7)       , 10>
    , arch_trait<STR(v6-m)     , 11>
    , arch_trait<STR(v6s-m)    , 12>
    , arch_trait<STR(v7e-m)    , 13>
    , arch_trait<STR(v8-a)     , 14>
    , arch_trait<STR(v8-r)     , 15>
    , arch_trait<STR(v8-m.baseline), 16>
    , arch_trait<STR(v8-m.mainline), 17>
    , arch_trait<STR(v8.1-a)   , 18>
    , arch_trait<STR(v8.2-a)   , 19>
    , arch_trait<STR(v8.3-a)   , 20>
    , arch_trait<STR(v8.1-m.mainline), 21>
    >;


}

#undef STR
#endif
