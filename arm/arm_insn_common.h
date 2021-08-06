#ifndef KAS_ARM_ARM_INSN_COMMON_H
#define KAS_ARM_ARM_INSN_COMMON_H

// arm instruction definion patterns
//
// see `target/tgt_insn_common.h` for description of
// insn definition pattern.

#include "arm_formats.h"            // actual format types
#include "arm_validate.h"           // actual validate types
#include "arm_mcode_sizes.h"        // define ARM insn variants

#include "target/tgt_insn_common.h"

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
// Naming scheme: lower case allow flag, upper case require flag

// u: condition field not allowed
// c: support for `cond` : allow condition field 
// n: support for `no-al`: don't allow `al` case (implies allow condition)
// s: support for `S`: update flags
using a7_u   = arm_sz<SZ_ARCH_ARM>;
using a7_c   = arm_sz<SZ_ARCH_ARM, SZ_DEFN_COND>;
using a7_cs  = arm_sz<SZ_ARCH_ARM, SZ_DEFN_COND               , SZ_DEFN_S_FLAG>;
using a7_n   = arm_sz<SZ_ARCH_ARM, SZ_DEFN_COND, SZ_DEFN_NO_AL>;
using a7_ns  = arm_sz<SZ_ARCH_ARM, SZ_DEFN_COND, SZ_DEFN_NO_AL, SZ_DEFN_S_FLAG>;

// declare "suffixes" for A7 instructions
// lower case implies sfx is optional, upper case requires suffix
using a7_cb  = arm_sz<SZ_ARCH_ARM, SZ_DEFN_COND, SZ_DEFN_B_FLAG>;
using a7_cT  = arm_sz<SZ_ARCH_ARM, SZ_DEFN_COND, SZ_DEFN_REQ_T>;
using a7_cHs = arm_sz<SZ_ARCH_ARM, SZ_DEFN_COND, SZ_DEFN_REQ_H, SZ_DEFN_S_FLAG>;
using a7_cM  = arm_sz<SZ_ARCH_ARM, SZ_DEFN_COND, SZ_DEFN_REQ_M>;
using a7_uM  = arm_sz<SZ_ARCH_ARM,               SZ_DEFN_REQ_M>;
using a7_uI  = arm_sz<SZ_ARCH_ARM,               SZ_DEFN_REQ_I>;
using a7_cl  = arm_sz<SZ_ARCH_ARM, SZ_DEFN_COND, SZ_DEFN_L_FLAG>;
using a7_ul  = arm_sz<SZ_ARCH_ARM,               SZ_DEFN_L_FLAG>;

// map "suffix" flag `sz` types to `info` manipulation types
using arm_sz_info_map = meta::list<
      meta::pair<a7_c       , struct arm_info_a7_c>
    , meta::pair<a7_n       , struct arm_info_a7_c>
    , meta::pair<a7_cs      , struct arm_info_a7_cs>
    , meta::pair<a7_ns      , struct arm_info_a7_cs>
    , meta::pair<a7_cb      , struct arm_info_a7_c_sfx>
    , meta::pair<a7_cT      , struct arm_info_a7_c_sfx>
    >;

// forward declare "list" info_fn
struct arm_info_list;


using ARMv6 = meta::int_<0>;

template <typename...Ts>
using defn = tgt_insn_defn<arm_mcode_t, arm_sz_info_map, Ts...>;

}


#endif

