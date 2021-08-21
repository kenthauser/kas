#ifndef ARM_ARM_DATA_PSEUDO_DEF_H
#define ARM_ARM_DATA_PSEUDO_DEF_H

#include "parser/parser_types.h"
#include "kas/kas_string.h"
#include <meta/meta.hpp>

namespace kas::arm::data_pseudo
{
using namespace meta;

#define STR(_)  KAS_STRING(#_)

using pseudo_ops_fixed = list<list<>
, list<int8_t   , STR(byte)>
, list<int16_t  , STR(half)>
, list<int32_t  , STR(word)>
, list<int64_t  , STR(quad)>
>;

}

namespace kas::parser::detail
{
    template <> struct pseudo_ops_fixed<void> : arm::data_pseudo::pseudo_ops_fixed {};
//    template <> pseudo_ops_float<defn_cpu> : 
}
#undef STR

#endif
