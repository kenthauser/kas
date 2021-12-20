#ifndef KAS_ARM_ARM_INSN_COMMON_H
#define KAS_ARM_ARM_INSN_COMMON_H

// arm instruction definion patterns
//
// see `target/tgt_insn_common.h` for description of
// insn definition pattern.

#include "arm7_formats.h"           // actual format types
#include "arm_validate.h"           // actual validate types
#include "arm_mcode.h"              // define ARM insn variants

#include "target/tgt_insn_common.h"

namespace kas::arm::opc
{
using namespace tgt::opc;
using namespace tgt::opc::traits;


// declare opcode groups (ie: include files)
using arm_insn_defn_groups = meta::list<
      struct OP_ARM_ARM5
    , struct OP_ARM_ARM6
    , struct OP_ARM_ARM7
    , struct OP_ARM_ARM8
    , struct OP_ARM_THUMB
    , struct OP_ARM_THUMB_32
    >;

// boilerplate: incorporate opcode groups
template <typename=void> struct arm_insn_defn_list : meta::list<> {};

// EXAMPLE: define `sz` types for first arg of `defn<>` template.
//          `sz_void` defined by base. All others are per-arch
// Naming scheme: lower case allow flag, upper case require flag

template <int OP_ARCH, int...OP_FLAGS>
using arm_sz = meta::int_<(OP_ARCH | ... | OP_FLAGS)>;


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

// base defns for thumb ops
// suffix naming matches `ARM7` patterns
using t1_u   = arm_sz<SZ_ARCH_THB>;
using t1_c   = arm_sz<SZ_ARCH_THB, SZ_DEFN_COND>;
using t1_uS  = arm_sz<SZ_ARCH_THB, SZ_DEFN_REQ_S>;
using t1_uB  = arm_sz<SZ_ARCH_THB, SZ_DEFN_REQ_B>;
using t1_uH  = arm_sz<SZ_ARCH_THB, SZ_DEFN_REQ_H>;
using t1_uSB = arm_sz<SZ_ARCH_THB, SZ_DEFN_REQ_SB>;
using t1_uSH = arm_sz<SZ_ARCH_THB, SZ_DEFN_REQ_SH>;

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

