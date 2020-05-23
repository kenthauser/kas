#ifndef KAS_Z80_Z80_HW_DEFNS_H
#define KAS_Z80_Z80_HW_DEFNS_H

#include "kas_core/hardware_features.h"
#include "kas_core/hardware_defns.h"
#include "kas/kas_string.h"

namespace kas::z80::hw
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

// declare features as types
#include "z80_features.inc"

using cpu_feature_types = pop_front<list<void
#define CPU_FEATURE(name) ,name
#include "z80_features.inc"
    >>;

// create alias for feature definitons (set `Feature Type List`)
template <typename...Args>
using cpu_defn = hw_features<cpu_feature_types, Args...>;

//////////////////////////////////////////////////////////////////////////////
//
// Processor (instruction set) Definitions
//
//////////////////////////////////////////////////////////////////////////////
//
// Declare hierarchy of instruction sets
//
//////////////////////////////////////////////////////////////////////////////

#define STR KAS_STRING

// base instructions & address modes supported

// enable m68k address modes not supported by coldfire
using z80   = cpu_defn<STR("z80"), list<>>;

//////////////////////////////////////////////////////////////////////////////
//
// These types can be used in opcode selection tests
//

// first is default. all listed are available on command line
using cpu_is_list = list<
              z80        // default
              // list of is-a cpus
            , z80 
            >;

//////////////////////////////////////////////////////////////////////////////
//
// Coprocessor Definitions
//
//////////////////////////////////////////////////////////////////////////////

using z80_hw_defs = hw_defs<cpu_is_list>;
using hw_tst      = typename z80_hw_defs::hw_tst;

extern z80_hw_defs z80_defs;

}
#endif
