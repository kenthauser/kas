#ifndef KAS_ARM_ARM_HW_DEFNS_H
#define KAS_ARM_ARM_HW_DEFNS_H

#include "kas_core/hardware_features.h"
#include "kas_core/hardware_defns.h"
#include "kas/kas_string.h"

namespace kas::arm::hw
{
using namespace core::hardware;
using namespace meta;

//////////////////////////////////////////////////////////////////////////////
//
// declare "features" as
//   - types
//   - member of `Feature Type List`

#define CPU_FEATURE(_name) struct _name { using name = KAS_STRING(#_name); }; 
#define THB_FEATURE  CPU_FEATURE
#define FPU_FEATURE  CPU_FEATURE
#include "arm_features.inc"

using cpu_feature_types = pop_front<list<void
#define CPU_FEATURE(name) ,name
#include "arm_features.inc"
    >>;

using thb_feature_types = pop_front<list<void
#define THB_FEATURE(name) ,name
#include "arm_features.inc"
    >>;

using fpu_feature_types = pop_front<list<void
#define FPU_FEATURE(name) ,name
#include "arm_features.inc"
    >>;


// create alias for feature definitons (set `Feature Type List`)
template <typename...Args>
using cpu_defn = hw_features<cpu_feature_types, Args...>;

template <typename...Args>
using thb_defn = hw_features<thb_feature_types, Args...>;

template <typename...Args>
using fpu_defn = hw_features<fpu_feature_types, Args...>;

//////////////////////////////////////////////////////////////////////////////
//
// Processor (instruction set) Definitions
//
//////////////////////////////////////////////////////////////////////////////
//
// Declare hierarchy of instruction sets. `arm_base` is base type, supported
// by all processors. From this derive the `arm` tree.
// Processors are related by c++ class hierarchy. Each processor
// can add & delete features, but NB: deleted features are sticky and can
// not be re-enabled in derived processors (classes).
//
//////////////////////////////////////////////////////////////////////////////

#define STR KAS_STRING

// enable arm features
using arm_base   = cpu_defn<STR("arm_base"), list<>
                    >;

using arm_v4     = cpu_defn<STR("arm_v4"), arm_base>;
using arm_v4t    = cpu_defn<STR("arm_v4t"), arm_v4>;

using arm_v5     = cpu_defn<STR("arm_v5"), arm_v4t>;
using arm_v5t    = cpu_defn<STR("arm_v5t"), arm_v5>;
using arm_v5te   = cpu_defn<STR("arm_v5te"), arm_v5t>;
using arm_v5tej  = cpu_defn<STR("arm_v5tej"), arm_v5te>;

using arm_v6     = cpu_defn<STR("arm_v6"), arm_v5tej>;
using arm_v6k    = cpu_defn<STR("arm_v6k"), arm_v6>;
using arm_v6t2   = cpu_defn<STR("arm_v6t2"), arm_v6>;

using arm_v7     = cpu_defn<STR("arm_v7"), arm_v6t2>;
using arm_v7a    = cpu_defn<STR("arm_v7a"), arm_v7>;
using arm_v7r    = cpu_defn<STR("arm_v7r"), arm_v7>;
using arm_v7m    = cpu_defn<STR("arm_v7m"), arm_v7>;

//
// These types can be used in opcode selection tests
//

// first is default. all listed are available on command line
using cpu_is_list = list<
              arm_v7    // default
            , arm_v4
            , arm_v4t
            , arm_v5
            , arm_v5t
            , arm_v5te
            , arm_v5tej
            , arm_v6
            , arm_v6k
            , arm_v6t2
            , arm_v7
            , arm_v7a
            , arm_v7r
            , arm_v7m
            >;


using cpu_defs_t = hw_defs<cpu_is_list>;
using hw_tst  = typename cpu_defs_t::hw_tst;
//using hw_void = typename cpu_defs_t::hw_void;
using hw_void  = std::integral_constant<int, 0>;
extern cpu_defs_t cpu_defs;

}
#endif
