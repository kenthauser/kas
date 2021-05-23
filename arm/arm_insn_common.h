#ifndef KAS_ARM_ARM_INSN_COMMON_H
#define KAS_ARM_ARM_INSN_COMMON_H

// arm instruction definion patterns
//
// see `target/tgt_insn_common.h` for description of
// insn definition pattern.

#include "arm_formats.h"            // actual format types
#include "arm_validate.h"           // actual validate types
#include "arm_mcode_sizes.h"        // defin ARM insn variants

#include "target/tgt_insn_common.h"  // declare "trait" for definition

namespace kas::arm::opc
{
using namespace tgt::opc;
using namespace tgt::opc::traits;


// declare opcode groups (ie: include files)
using arm_insn_defn_groups = meta::list<
      struct OP_ARM_GEN
    >;

// boilerplate: incorporate opcode groups
template <typename=void> struct arm_insn_defn_list : meta::list<> {};

// EXAMPLE: define `sz` types for first arg of `defn<>` template.
//          `sz_void` defined by base. All others are per-arch
using a7_u   = arm_sz<SZ_ARCH_ARM>;
using a7_c   = arm_sz<SZ_ARCH_ARM, SZ_DEFN_COND>;
using a7_cs  = arm_sz<SZ_ARCH_ARM, SZ_DEFN_COND               , SZ_DEFN_S_FLAG>;
using a7_n   = arm_sz<SZ_ARCH_ARM, SZ_DEFN_COND, SZ_DEFN_NO_AL                >;
using a7_ns  = arm_sz<SZ_ARCH_ARM, SZ_DEFN_COND, SZ_DEFN_NO_AL, SZ_DEFN_S_FLAG>;

// declare "sizes" for A7 ldr/str instructions
using a7_cb  = arm_sz<SZ_ARCH_ARM, SZ_DEFN_COND, SZ_DEFN_B_FLAG               >;
using a7_cT  = arm_sz<SZ_ARCH_ARM, SZ_DEFN_COND, SZ_DEFN_REQ_T                >;
using a7_cHs = arm_sz<SZ_ARCH_ARM, SZ_DEFN_COND, SZ_DEFN_REQ_H, SZ_DEFN_S_FLAG>;
using a7_cM  = arm_sz<SZ_ARCH_ARM, SZ_DEFN_COND, SZ_DEFN_REQ_M                >;

// support for `cond` : allow condition field 
// support for `no-al`: don't allow `al` case (implies allow condition)
// support for `S`: update flags
}


#endif

