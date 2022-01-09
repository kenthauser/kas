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
    , struct OP_ARM_THUMB_16
    , struct OP_ARM_THUMB_32
    , struct OP_ARM_THUMB_EE
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
using a32_u   = arm_sz<SZ_ARCH_ARM>;
using a32_c   = arm_sz<SZ_ARCH_ARM, SZ_DEFN_COND>;
using a32_cs  = arm_sz<SZ_ARCH_ARM, SZ_DEFN_COND               , SZ_DEFN_S_FLAG>;
using a32_n   = arm_sz<SZ_ARCH_ARM, SZ_DEFN_COND, SZ_DEFN_NO_AL>;
using a32_ns  = arm_sz<SZ_ARCH_ARM, SZ_DEFN_COND, SZ_DEFN_NO_AL, SZ_DEFN_S_FLAG>;

// declare "suffixes" for A7 instructions
// lower case implies sfx is optional, upper case requires suffix
using a32_cb  = arm_sz<SZ_ARCH_ARM, SZ_DEFN_COND, SZ_DEFN_B_FLAG>;
using a32_cT  = arm_sz<SZ_ARCH_ARM, SZ_DEFN_COND, SZ_DEFN_REQ_T>;
using a32_cHs = arm_sz<SZ_ARCH_ARM, SZ_DEFN_COND, SZ_DEFN_REQ_H, SZ_DEFN_S_FLAG>;
using a32_cM  = arm_sz<SZ_ARCH_ARM, SZ_DEFN_COND, SZ_DEFN_REQ_M>;
using a32_uM  = arm_sz<SZ_ARCH_ARM,               SZ_DEFN_REQ_M>;
using a32_uI  = arm_sz<SZ_ARCH_ARM,               SZ_DEFN_REQ_I>;
using a32_cl  = arm_sz<SZ_ARCH_ARM, SZ_DEFN_COND, SZ_DEFN_L_FLAG>;
using a32_ul  = arm_sz<SZ_ARCH_ARM,               SZ_DEFN_L_FLAG>;

// base defns for thumb ops
// suffix naming matches `ARM7` patterns
using t16_u   = arm_sz<SZ_ARCH_THB16>;
using t16_c   = arm_sz<SZ_ARCH_THB16, SZ_DEFN_COND>;
using t16_uS  = arm_sz<SZ_ARCH_THB16, SZ_DEFN_REQ_S>;
using t16_uB  = arm_sz<SZ_ARCH_THB16, SZ_DEFN_REQ_B>;
using t16_uH  = arm_sz<SZ_ARCH_THB16, SZ_DEFN_REQ_H>;
using t16_uSB = arm_sz<SZ_ARCH_THB16, SZ_DEFN_REQ_SB>;
using t16_uSH = arm_sz<SZ_ARCH_THB16, SZ_DEFN_REQ_SH>;

// base defns for thumb ops
// suffix naming matches `ARM7` patterns
using t32_u   = arm_sz<SZ_ARCH_THB32>;
using t32_c   = arm_sz<SZ_ARCH_THB32, SZ_DEFN_COND>;
using t32_n   = arm_sz<SZ_ARCH_THB32, SZ_DEFN_COND, SZ_DEFN_NO_AL>;
using t32_cs  = arm_sz<SZ_ARCH_THB32, SZ_DEFN_COND, SZ_DEFN_S_FLAG>;
using t32_cS  = arm_sz<SZ_ARCH_THB32, SZ_DEFN_COND, SZ_DEFN_REQ_S>;

// ldr/str multiple has addressing mode suffixes: support pre-UAL parsing
// NB: none of these codes require base code modification
using t32_cd  = t32_c;  // pop default : allow load descending: M-FLAG (IA, FD)
using t32_cA  = t32_c;  // push_default: require load ascending: M_FLAG (DB, FD)
using t32_cD  = t32_c;  // require store descending: M-FLAG (DB, EA)
using t32_ca  = t32_c;  // allow store ascending:    M-FLAG (IA, EA)


using t32_ct  = t32_c;

using t32_uB  = arm_sz<SZ_ARCH_THB32, SZ_DEFN_REQ_B>;
using t32_uH  = arm_sz<SZ_ARCH_THB32, SZ_DEFN_REQ_H>;
using t32_uSB = arm_sz<SZ_ARCH_THB32, SZ_DEFN_REQ_SB>;
using t32_uSH = arm_sz<SZ_ARCH_THB32, SZ_DEFN_REQ_SH>;

// declare "suffixes" for T32 instructions
// lower case implies sfx is optional, upper case requires suffix
using t32_cb  = arm_sz<SZ_ARCH_ARM, SZ_DEFN_COND, SZ_DEFN_B_FLAG>;
using t32_cT  = arm_sz<SZ_ARCH_ARM, SZ_DEFN_COND, SZ_DEFN_REQ_T>;
using t32_cHs = arm_sz<SZ_ARCH_ARM, SZ_DEFN_COND, SZ_DEFN_REQ_H, SZ_DEFN_S_FLAG>;
using t32_cM  = arm_sz<SZ_ARCH_ARM, SZ_DEFN_COND, SZ_DEFN_REQ_M>;
using t32_uM  = arm_sz<SZ_ARCH_ARM,               SZ_DEFN_REQ_M>;
using t32_uI  = arm_sz<SZ_ARCH_ARM,               SZ_DEFN_REQ_I>;
using t32_cl  = arm_sz<SZ_ARCH_ARM, SZ_DEFN_COND, SZ_DEFN_L_FLAG>;
using t32_ul  = arm_sz<SZ_ARCH_ARM,               SZ_DEFN_L_FLAG>;

using t32_cH = t32_c;

// map "suffix" flag `sz` types to `info` manipulation types
using arm_sz_info_map = meta::list<
      meta::pair<a32_c       , struct arm_info_a32_c>
    , meta::pair<a32_n       , struct arm_info_a32_c>
    , meta::pair<a32_cs      , struct arm_info_a32_cs>
    , meta::pair<a32_ns      , struct arm_info_a32_cs>
    , meta::pair<a32_cb      , struct arm_info_a32_c_sfx>
    , meta::pair<a32_cT      , struct arm_info_a32_c_sfx>
    >;

// forward declare "list" info_fn
struct arm_info_list;


using ARMv6 = meta::int_<0>;

template <typename...Ts>
using defn = tgt_insn_defn<arm_mcode_t, arm_sz_info_map, Ts...>;

}


#endif

