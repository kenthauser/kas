#ifndef KAS_ARM_ARM_INSN_COMMON_H
#define KAS_ARM_ARM_INSN_COMMON_H

// arm instruction definion patterns
//
// see `target/tgt_mcode_defn_trait.h` for description of
// insn definition pattern.

#include "arm_format.h"             // actual format types
#include "arm_validate.h"           // actual validate types
#include "arm_mcode_sizes.h"        // define ARM insn variants

#include "target/tgt_insn_common.h" // declare "trait" for definition

namespace kas::arm::opc
{

// declare opcode groups (ie: include files)
using arm_insn_defn_groups = meta::list<
      struct OP_ARM_GEN
    >;

#if 0
template <std::size_t CODE, typename TST = void, typename MASK = 0>
using OP = tgt::opc::traits::OP<CODE, TST, void, MASK>;
#endif

template <typename=void> struct arm_insn_defn_list : meta::list<> {};

using namespace tgt::opc;
using namespace tgt::opc::traits;

using sz_v    = void;
using sz_w    = meta::int_<0>;

using a7_u    = arm_sz<SZ_ARCH_ARM>;
using a7_c    = arm_sz<SZ_ARCH_ARM, SZ_DEFN_COND>;
using a7_cq   = arm_sz<SZ_ARCH_ARM, SZ_DEFN_COND, SZ_DEFN_NW_FLAG>;
using a7_cqs  = arm_sz<SZ_ARCH_ARM, SZ_DEFN_COND, SZ_DEFN_NW_FLAG, SZ_DEFN_S_FLAG>;
using a7_nq   = arm_sz<SZ_ARCH_ARM, SZ_DEFN_COND, SZ_DEFN_NW_FLAG, SZ_DEFN_NO_AL>;
using a7_nqs  = arm_sz<SZ_ARCH_ARM, SZ_DEFN_COND, SZ_DEFN_NW_FLAG, SZ_DEFN_S_FLAG, SZ_DEFN_NO_AL>;
using a7_q    = arm_sz<SZ_ARCH_ARM, SZ_DEFN_NW_FLAG>;

// support for `cond` : add condition field 
// support for `no-al`: don't add `al` case
// support for `S`: update sign
// add support for `.n` & `.w` (narrow or wide)

}


#endif

