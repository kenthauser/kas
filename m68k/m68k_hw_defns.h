#ifndef KAS_M68K_M68K_HW_DEFNS_H
#define KAS_M68K_M68K_HW_DEFNS_H

#include "kas_core/hardware_features.h"
#include "kas_core/hardware_defns.h"
#include "kas/kas_string.h"

namespace kas::m68k::hw
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
#include "m68k_features.inc"

using cpu_feature_types = pop_front<list<void
#define CPU_FEATURE(name) ,name
#include "m68k_features.inc"
    >>;

using fpu_feature_types = pop_front<list<void
#define FPU_FEATURE(name) ,name
#include "m68k_features.inc"
    >>;

using mmu_feature_types = pop_front<list<void
#define MMU_FEATURE(name) ,name
#include "m68k_features.inc"
    >>;

// create alias for feature definitons (set `Feature Type List`)
template <typename...Args>
using cpu_defn = hw_features<cpu_feature_types, Args...>;

template <typename...Args>
using fpu_defn = hw_features<fpu_feature_types, Args...>;

template <typename...Args>
using mmu_defn = hw_features<mmu_feature_types, Args...>;

//////////////////////////////////////////////////////////////////////////////
//
// Processor (instruction set) Definitions
//
//////////////////////////////////////////////////////////////////////////////
//
// Declare hierarchy of instruction sets. `m68kcf` is base type, supported
// by all processors. From this derive `m68k` tree & the `coldfire` tree.
// Processors are related by c++ class hierarchy. Each processor
// can add & delete features, but NB: deleted features are sticky and can
// not be re-enabled in derived processors (classes).
//
//////////////////////////////////////////////////////////////////////////////

#define STR KAS_STRING

// base instructions & address modes supported by m68000 & coldfire ISA_A
// `limit_3w` limits instructions to three words total (coldfire)
// `math` identifies math instructions which only support long (coldfire)
using m68kcf = cpu_defn<STR("base"), list<>, math, limit_3w>;

// enable m68k address modes not supported by coldfire
using m68k   = cpu_defn<STR("m68k"), m68kcf
                    , list<index_word           // added features
                         , movep
                         , divide
                         >
                    , list<limit_3w>            // deleted features
                    >;

// declare processor hierarchy: features typically inherited after introduction
using m68000 = cpu_defn<STR("m68000"), m68k>;
using m68010 = cpu_defn<STR("m68010"), m68000, read_ccr>;

using m68020 = cpu_defn<STR("m68020"), m68010
                    // '020 added lots of features
                    , fpu                       // match default setting of `gas`
                    , branch_long
                    , callm                     // 68020 only
                    , mult64
                    , index_full
                    , index_scale
                    , index_scale_8
                    >;

using m68030 = cpu_defn<STR("m68030"), m68020
                    , list<mmu>                 // added features
                    , list<callm>               // deleted features
                    >;

using m68040 = cpu_defn<STR("m68040"), m68030, fpu>;
using m68060 = cpu_defn<STR("m68060"), m68040
                    , list<>
                    , list<movep>               // deleted feature
                    >;

// add reduced feature set types
using m68ec000 = cpu_defn<STR("m68EC000"), m68000, read_ccr>;

using m68lc040 = cpu_defn<STR("m68LC040"), m68040, list<>, list<fpu>>;
using m68ec040 = cpu_defn<STR("m68EC040"), m68lc040, list<>, list<mmu>>;

using m68lc060 = cpu_defn<STR("m68LC060"), m68060, list<>, list<fpu>>;
using m68ec060 = cpu_defn<STR("m68EC060"), m68lc060, list<>, list<mmu>>;

// cpu32 derived from EC000, not 68000. Supports table insns
using cpu32    = cpu_defn<STR("cpu32"), m68ec000
                    , index_scale
                    , index_scale_8
                    , table
                    >;

// dragonball (aka mc68328) is just 'EC000
using dragonball = cpu_defn<STR("dragonball"), m68ec000>;

//////////////////////////////////////////////////////////////////////////////
//
// related architectures

// coldfire supports index scaled address mode and an multiplying accumulator
// this is `pre ISA_A`: ie no support for divide
using coldfire = cpu_defn<STR("coldfire"), m68kcf
                    , read_ccr
                    , index_scale
                    , mult64
                    , mac
                    , fpu
                    , table
                    , mmu
                    >;
                    
// isa_a is the initial coldfire architecture
using isa_a   = cpu_defn<STR("isa_a"), coldfire, divide>;

// isa_a+ adds a few instructions, which are then deleted in isa_b
using isa_ap  = cpu_defn<STR("isa_a+"), isa_a>;

// isa_b is the successor to isa_a
using isa_b   = cpu_defn<STR("isa_b"),  isa_a>;

// isa_c is the successor to isa_b, but adds the isa_a+ insns
using isa_c   = cpu_defn<STR("isa_c"),  list<isa_b, isa_ap>
                    , list<emac>            // added features
                    , list<>                // deleted features
                    >;

// DEVELOPMENT TARGET: doesn't test mac
//using cpu_test = cpu_defn<STR("cpu_test"), list<isa_c, m68040>>;


//////////////////////////////////////////////////////////////////////////////
//
// These types can be used in opcode selection tests
//

// first is default. all listed are available on command line
using cpu_is_list = list<
              m68020        // default
              //cpu_test
            , m68000
            , m68010
            , m68020
            , m68030
            , m68040
            , m68060
            
            // EC versions
            , m68ec000
            , m68ec040
            , m68ec060
            
            // LC versions 
            , m68lc040
            , m68lc060
            
            // misc 68K variants
            , cpu32
            , dragonball
            
            // base insns
            , m68k
            , coldfire

            // coldfire ISAs
            , isa_a
            , isa_ap
            , isa_b
            , isa_c
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

using fpu_base = fpu_defn<void, list<>>;

// enable m68k address modes not supported by coldfire
using m68881   = fpu_defn<STR("m68881"), fpu_base
                    , fpu_basic
                    , fpu_m68k
                    , fpu_trig
                    , fpu_p_addr
                    , fpu_x_addr
                    , fpu_intrz
                    >;

using m68882   = fpu_defn<STR("m68882"), m68881>;

using fpu_m68030 = fpu_defn<STR("m68030"), fpu_base
                    , fpu_basic
                    , fpu_basic_rnd
                    , fpu_m68k
                    , fpu_m68k_rnd
                    , fpu_x_addr
                    >;

using fpu_coldfire = fpu_defn<STR("coldfire"), fpu_base
                    , fpu_basic
                    , fpu_basic_rnd
                    , fpu_m68k
                    , fpu_m68k_rnd
                    , fpu_intrz
                    >;


// select FPU based on CPU
// Default is 68881
template <typename CPU = void> struct fpu4cpu : m68881 {};

// Select internal FPU based on cpu.
template <> struct fpu4cpu<m68030>    : fpu_m68030 {};
template <> struct fpu4cpu<coldfire>  : fpu_coldfire {};

//////////////////////////////////////////////////////////////////////////////
//
// Memory manager coprocessor
//
//////////////////////////////////////////////////////////////////////////////

using mmu_base = mmu_defn<STR("mmu_base"), list<>>;

// declare various types of MMU
using m68551   = mmu_defn<STR("m68551"), mmu_base
                    >;


// select FPU based on CPU
// default is 68551
template <typename CPU = void> struct mmu4cpu : m68551 {};

// override based on CPU
//template <> struct mmu4cpu<m68030>     { using type = mmu_m68030;   };
//template <> struct mmu4cpu<coldfire>   { using type = mmu_coldfire; };


//////////////////////////////////////////////////////////////////////////////
//
// Combine everything into type for `core::hardware_defns`
//
//////////////////////////////////////////////////////////////////////////////

using fpu_cp_defn = hw_cp_def<fpu, fpu4cpu>;
using mmu_cp_defn = hw_cp_def<mmu, mmu4cpu>;

using cpu_defs_t = hw_defs<cpu_is_list, fpu_cp_defn, mmu_cp_defn>;
using hw_tst  = typename cpu_defs_t::hw_tst;
//using hw_void = typename cpu_defs_t::hw_void;
using hw_void  = std::integral_constant<int, 0>;
extern cpu_defs_t cpu_defs;

}
#endif
