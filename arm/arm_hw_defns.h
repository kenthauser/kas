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
#define FPU_FEATURE  CPU_FEATURE
#define MMU_FEATURE  CPU_FEATURE
#define THB_FEATURE  CPU_FEATURE
#include "arm_features.inc"

using cpu_feature_types = pop_front<list<void
#define CPU_FEATURE(name) ,name
#include "arm_features.inc"
    >>;

using fpu_feature_types = pop_front<list<void
#define FPU_FEATURE(name) ,name
#include "arm_features.inc"
    >>;

using mmu_feature_types = pop_front<list<void
#define MMU_FEATURE(name) ,name
#include "arm_features.inc"
    >>;

using thb_feature_types = pop_front<list<void
#define THB_FEATURE(name) ,name
#include "arm_features.inc"
    >>;

// create alias for feature definitons (set `Feature Type List`)
template <typename...Args>
using cpu_defn = hw_features<cpu_feature_types, Args...>;

template <typename...Args>
using fpu_defn = hw_features<fpu_feature_types, Args...>;

template <typename...Args>
using mmu_defn = hw_features<mmu_feature_types, Args...>;

template <typename...Args>
using thb_defn = hw_features<thb_feature_types, Args...>;

//////////////////////////////////////////////////////////////////////////////
//
// Processor (instruction set) Definitions
//
//////////////////////////////////////////////////////////////////////////////
//
// Declare hierarchy of instruction sets. `arm_base` is base type, supported
// by all processors. From this derive the `arm` tree.
//
// Processors are related by c++ class hierarchy. Each processor
// can add & delete features, but NB: deleted features are sticky and can
// not be re-enabled in derived processors (classes).
//
//////////////////////////////////////////////////////////////////////////////

#define STR KAS_STRING

// enable arm features
using arm_base = cpu_defn<STR("arm_base"), list<>>;
 
using v4     = cpu_defn<STR("arm_v4"), arm_base>;
using v4t    = cpu_defn<STR("arm_v4t"), v4>;

using v5     = cpu_defn<STR("arm_v5"), v4t>;
using v5t    = cpu_defn<STR("arm_v5t"), v5>;
using v5te   = cpu_defn<STR("arm_v5te"), v5t>;
using v5tej  = cpu_defn<STR("arm_v5tej"), v5te>;

using v6     = cpu_defn<STR("arm_v6"), v5tej>;
using v6k    = cpu_defn<STR("arm_v6k"), v6>;
using v6t2   = cpu_defn<STR("arm_v6t2"), v6>;

using v7     = cpu_defn<STR("arm_v7"), v6t2>;
using v7a    = cpu_defn<STR("arm_v7a"), v7>;
using v7r    = cpu_defn<STR("arm_v7r"), v7>;
using v7m    = cpu_defn<STR("arm_v7m"), v7>;
 

//////////////////////////////////////////////////////////////////////////////
//
// These types can be used in opcode selection tests
//

// first is default. all listed are available on command line
using cpu_is_list = list<
              v7    // default
            , v4
            , v4t
            , v5
            , v5t
            , v5te
            , v5tej
            , v6
            , v6k
            , v6t2
            , v7
            , v7a
            , v7r
            , v7m
            >;

//////////////////////////////////////////////////////////////////////////////
//
// Coprocessor Definitions
//
//////////////////////////////////////////////////////////////////////////////
//
// Floating point coprocessor
//
//////////////////////////////////////////////////////////////////////////////

// base FPU is no FPU
using fpu_base = fpu_defn<void, list<>>;


// select FPU based on CPU
// Default is fpu_base 
template <typename CPU = void> struct fpu4cpu : fpu_base {};

// Select internal FPU based on cpu.
//template <> struct fpu4cpu<xxx>     { using type = fpu_xxx;   };

//////////////////////////////////////////////////////////////////////////////
//
// Memory manager coprocessor
//
//////////////////////////////////////////////////////////////////////////////

// base MMU is no MMU
using mmu_base = mmu_defn<STR("mmu_base"), list<>>;

// select FPU based on CPU
// default is mmu_base 
template <typename CPU = void> struct mmu4cpu : mmu_base {};

// override based on CPU
//template <> struct mmu4cpu<xxx>     { using type = mmu_xxx;   };


//////////////////////////////////////////////////////////////////////////////
//
// Combine everything into type for `core::hardware_defns`
//
//////////////////////////////////////////////////////////////////////////////

using fpu_cp_defn = hw_cp_def<fpu, fpu4cpu>;
using mmu_cp_defn = hw_cp_def<mmu, mmu4cpu>;

using arm_hw_defs = hw_defs<cpu_is_list, fpu_cp_defn, mmu_cp_defn>;
using cpu_defs_t  = arm_hw_defs;
using hw_tst  = typename cpu_defs_t::hw_tst;
using hw_void  = std::integral_constant<int, 0>;
extern cpu_defs_t cpu_defs;

}
#endif
