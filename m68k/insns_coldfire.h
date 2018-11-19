#ifndef KAS_M68K_INSNS_COLDFIRE_H
#define KAS_M68K_INSNS_COLDFIRE_H

#include "m68k_insn_common.h"

namespace kas::m68k::opc::ns_coldfire
{

#define STR KAS_STRING

using cf_mac_moves_v = list<list<>
// generic mac
, insn<sz_l, STR("move"), OP<0xa980, coldfire>, FMT_X_0RM, MACSR,   GEN_REG>
, insn<sz_l, STR("move"), OP<0xa9c0, coldfire>, void,      MACSR,   CCR>
, insn<sz_l, STR("move"), OP<0xa900, coldfire>, FMT_0RM,   GEN_REG, MACSR>
, insn<sz_l, STR("move"), OP<0xa900, coldfire>, FMT_0RM,   IMMED,   MACSR>

, insn<sz_l, STR("move"), OP<0xad80, coldfire>, FMT_X_0RM, MASK,    GEN_REG>
, insn<sz_l, STR("move"), OP<0xad00, coldfire>, FMT_0RM,   GEN_REG, MASK>
, insn<sz_l, STR("move"), OP<0xad00, coldfire>, FMT_0RM,   IMMED,   MASK>

// original mac: access ACC
, insn<sz_l, STR("move"), OP<0xa180, mac>, FMT_X_0RM, ACC, GEN_REG>
, insn<sz_l, STR("move"), OP<0xa100, mac>, FMT_0RM,   GEN_REG, ACC>
, insn<sz_l, STR("move"), OP<0xa100, mac>, FMT_0RM,   IMMED, ACC>
    
// emac access ACC_N
// acc = w0b9
, insn<sz_l, STR("move"), OP<0xa180, emac>, FMT_9AN_0RM, ACC_N, GEN_REG>
, insn<sz_l, STR("move"), OP<0xa100, emac>, FMT_0RM_9AN, GEN_REG, ACC_N>
, insn<sz_l, STR("move"), OP<0xa100, emac>, FMT_0RM_9AN, IMMED, ACC_N>

// src = w0b0, dst = w0b9
, insn<sz_l, STR("move"), OP<0xa110, emac>, FMT_0AN_9AN, ACC_N, ACC_N>

// acc = w0b9
, insn<sz_l, STR("movclr"), OP<0xa1c0, emac>, FMT_9AN_0RM, ACC_N, GEN_REG>
    
// emac access ACC_EXT..
, insn<sz_l, STR("move"), OP<0xab80, emac>, FMT_X_0RM, ACC_EXT01, GEN_REG>
, insn<sz_l, STR("move"), OP<0xaf80, emac>, FMT_X_0RM, ACC_EXT23, GEN_REG>
, insn<sz_l, STR("move"), OP<0xab00, emac>, FMT_0RM, GEN_REG, ACC_EXT01>
, insn<sz_l, STR("move"), OP<0xaf00, emac>, FMT_0RM, GEN_REG, ACC_EXT23>
, insn<sz_l, STR("move"), OP<0xab00, emac>, FMT_0RM, IMMED, ACC_EXT01>
, insn<sz_l, STR("move"), OP<0xaf00, emac>, FMT_0RM, IMMED, ACC_EXT23>
>;

// the MAC (and eMAC) have many odd syntax quarks (eg optional << or >>)
// Also optional `&` after indirect arg to indicate MASK in use

// CAN do wl with INFO_SIZE_MAC
using cf_mac_v = list<list<>
// original mac
// dst msb = w0b6, src ul = w1b6, dst ul = w1b7
, insn<sz_w, STR("mac"),  OP<0xa000'0000, mac>, FMT_0UL_9UL, REG_UL, REG_UL>
, insn<sz_w, STR("mac"),  OP<0xa000'0200, mac>, FMT_0UL_9UL, REG_UL, REG_UL, SF_LEFT>
, insn<sz_w, STR("mac"),  OP<0xa000'0600, mac>, FMT_0UL_9UL, REG_UL, REG_UL, SF_RIGHT>
, insn<sz_l, STR("mac"),  OP<0xa000'0800, mac>, FMT_0UL_9UL, GEN_REG, GEN_REG>
, insn<sz_l, STR("mac"),  OP<0xa000'0a00, mac>, FMT_0UL_9UL, GEN_REG, GEN_REG, SF_LEFT>
, insn<sz_l, STR("mac"),  OP<0xa000'0e00, mac>, FMT_0UL_9UL, GEN_REG, GEN_REG, SF_RIGHT>
    
, insn<sz_w, STR("msac"),  OP<0xa000'0100, mac>, FMT_0UL_9UL, REG_UL, REG_UL>
, insn<sz_w, STR("msac"),  OP<0xa000'0300, mac>, FMT_0UL_9UL, REG_UL, REG_UL, SF_LEFT>
, insn<sz_w, STR("msac"),  OP<0xa000'0700, mac>, FMT_0UL_9UL, REG_UL, REG_UL, SF_RIGHT>
, insn<sz_l, STR("msac"),  OP<0xa000'0900, mac>, FMT_0UL_9UL, GEN_REG, GEN_REG>
, insn<sz_l, STR("msac"),  OP<0xa000'0b00, mac>, FMT_0UL_9UL, GEN_REG, GEN_REG, SF_LEFT>
, insn<sz_l, STR("msac"),  OP<0xa000'0f00, mac>, FMT_0UL_9UL, GEN_REG, GEN_REG, SF_RIGHT>

// INDIRECT doesn't match `MASK` versions. INDIR_MASK matches both.
// First to match is chosen...so order matters
, insn<sz_w, STR("macl"),  OP<0xa080'0000, mac>, FMT_UL0_UL12_0RM_9UL,   REG_UL, REG_UL, INDIRECT, GEN_REG>
, insn<sz_w, STR("macl"),  OP<0xa080'0020, mac>, FMT_UL0_UL12_0RM_9UL,   REG_UL, REG_UL, INDIR_MASK, GEN_REG>
, insn<sz_w, STR("macl"),  OP<0xa080'0200, mac>, FMT_UL0_UL12_X_0RM_9UL, REG_UL, REG_UL, SF_LEFT, INDIRECT, GEN_REG>
, insn<sz_w, STR("macl"),  OP<0xa080'0220, mac>, FMT_UL0_UL12_X_0RM_9UL, REG_UL, REG_UL, SF_LEFT, INDIR_MASK, GEN_REG>
, insn<sz_w, STR("macl"),  OP<0xa080'0600, mac>, FMT_UL0_UL12_X_0RM_9UL, REG_UL, REG_UL, SF_RIGHT, INDIRECT, GEN_REG>
, insn<sz_w, STR("macl"),  OP<0xa080'0620, mac>, FMT_UL0_UL12_X_0RM_9UL, REG_UL, REG_UL, SF_RIGHT, INDIR_MASK, GEN_REG>

, insn<sz_l, STR("macl"),  OP<0xa080'0800, mac>, FMT_UL0_UL12_0RM_9UL,   GEN_REG, GEN_REG, INDIRECT, GEN_REG>
, insn<sz_l, STR("macl"),  OP<0xa080'0820, mac>, FMT_UL0_UL12_0RM_9UL,   GEN_REG, GEN_REG, INDIR_MASK, GEN_REG>
, insn<sz_l, STR("macl"),  OP<0xa080'0a00, mac>, FMT_UL0_UL12_X_0RM_9UL, GEN_REG, GEN_REG, SF_LEFT, INDIRECT, GEN_REG>
, insn<sz_l, STR("macl"),  OP<0xa080'0a20, mac>, FMT_UL0_UL12_X_0RM_9UL, GEN_REG, GEN_REG, SF_LEFT, INDIR_MASK, GEN_REG>
, insn<sz_l, STR("macl"),  OP<0xa080'0e00, mac>, FMT_UL0_UL12_X_0RM_9UL, GEN_REG, GEN_REG, SF_RIGHT, INDIRECT, GEN_REG>
, insn<sz_l, STR("macl"),  OP<0xa080'0e20, mac>, FMT_UL0_UL12_X_0RM_9UL, GEN_REG, GEN_REG, SF_RIGHT, INDIR_MASK, GEN_REG>
    
, insn<sz_w, STR("msacl"),  OP<0xa080'0100, mac>, FMT_UL0_UL12_0RM_9UL,   REG_UL, REG_UL, INDIRECT, GEN_REG>
, insn<sz_w, STR("msacl"),  OP<0xa080'0120, mac>, FMT_UL0_UL12_0RM_9UL,   REG_UL, REG_UL, INDIR_MASK, GEN_REG>
, insn<sz_w, STR("msacl"),  OP<0xa080'0300, mac>, FMT_UL0_UL12_X_0RM_9UL, REG_UL, REG_UL, SF_LEFT, INDIRECT, GEN_REG>
, insn<sz_w, STR("msacl"),  OP<0xa080'0320, mac>, FMT_UL0_UL12_X_0RM_9UL, REG_UL, REG_UL, SF_LEFT, INDIR_MASK, GEN_REG>
, insn<sz_w, STR("msacl"),  OP<0xa080'0700, mac>, FMT_UL0_UL12_X_0RM_9UL, REG_UL, REG_UL, SF_RIGHT, INDIRECT, GEN_REG>
, insn<sz_w, STR("msacl"),  OP<0xa080'0720, mac>, FMT_UL0_UL12_X_0RM_9UL, REG_UL, REG_UL, SF_RIGHT, INDIR_MASK, GEN_REG>

, insn<sz_l, STR("msacl"),  OP<0xa080'0900, mac>, FMT_UL0_UL12_0RM_9UL,   GEN_REG, GEN_REG, INDIRECT, GEN_REG>
, insn<sz_l, STR("msacl"),  OP<0xa080'0920, mac>, FMT_UL0_UL12_0RM_9UL,   GEN_REG, GEN_REG, INDIR_MASK, GEN_REG>
, insn<sz_l, STR("msacl"),  OP<0xa080'0b00, mac>, FMT_UL0_UL12_X_0RM_9UL, GEN_REG, GEN_REG, SF_LEFT, INDIRECT, GEN_REG>
, insn<sz_l, STR("msacl"),  OP<0xa080'0b20, mac>, FMT_UL0_UL12_X_0RM_9UL, GEN_REG, GEN_REG, SF_LEFT, INDIR_MASK, GEN_REG>
, insn<sz_l, STR("msacl"),  OP<0xa080'0f00, mac>, FMT_UL0_UL12_X_0RM_9UL, GEN_REG, GEN_REG, SF_RIGHT, INDIRECT, GEN_REG>
, insn<sz_l, STR("msacl"),  OP<0xa080'0f20, mac>, FMT_UL0_UL12_X_0RM_9UL, GEN_REG, GEN_REG, SF_RIGHT, INDIR_MASK, GEN_REG>
    
// Manual calls ops `mac` but app notes use `macl`. Duplicate LOAD instructions w/o `L` suffix
, insn<sz_w, STR("mac"),  OP<0xa080'0000, mac>, FMT_UL0_UL12_0RM_9UL,   REG_UL, REG_UL, INDIRECT, GEN_REG>
, insn<sz_w, STR("mac"),  OP<0xa080'0020, mac>, FMT_UL0_UL12_0RM_9UL,   REG_UL, REG_UL, INDIR_MASK, GEN_REG>
, insn<sz_w, STR("mac"),  OP<0xa080'0200, mac>, FMT_UL0_UL12_X_0RM_9UL, REG_UL, REG_UL, SF_LEFT, INDIRECT, GEN_REG>
, insn<sz_w, STR("mac"),  OP<0xa080'0220, mac>, FMT_UL0_UL12_X_0RM_9UL, REG_UL, REG_UL, SF_LEFT, INDIR_MASK, GEN_REG>
, insn<sz_w, STR("mac"),  OP<0xa080'0600, mac>, FMT_UL0_UL12_X_0RM_9UL, REG_UL, REG_UL, SF_RIGHT, INDIRECT, GEN_REG>
, insn<sz_w, STR("mac"),  OP<0xa080'0620, mac>, FMT_UL0_UL12_X_0RM_9UL, REG_UL, REG_UL, SF_RIGHT, INDIR_MASK, GEN_REG>

, insn<sz_l, STR("mac"),  OP<0xa080'0800, mac>, FMT_UL0_UL12_0RM_9UL,   GEN_REG, GEN_REG, INDIRECT, GEN_REG>
, insn<sz_l, STR("mac"),  OP<0xa080'0820, mac>, FMT_UL0_UL12_0RM_9UL,   GEN_REG, GEN_REG, INDIR_MASK, GEN_REG>
, insn<sz_l, STR("mac"),  OP<0xa080'0a00, mac>, FMT_UL0_UL12_X_0RM_9UL, GEN_REG, GEN_REG, SF_LEFT, INDIRECT, GEN_REG>
, insn<sz_l, STR("mac"),  OP<0xa080'0a20, mac>, FMT_UL0_UL12_X_0RM_9UL, GEN_REG, GEN_REG, SF_LEFT, INDIR_MASK, GEN_REG>
, insn<sz_l, STR("mac"),  OP<0xa080'0e00, mac>, FMT_UL0_UL12_X_0RM_9UL, GEN_REG, GEN_REG, SF_RIGHT, INDIRECT, GEN_REG>
, insn<sz_l, STR("mac"),  OP<0xa080'0e20, mac>, FMT_UL0_UL12_X_0RM_9UL, GEN_REG, GEN_REG, SF_RIGHT, INDIR_MASK, GEN_REG>
    
, insn<sz_w, STR("msac"),  OP<0xa080'0100, mac>, FMT_UL0_UL12_0RM_9UL,   REG_UL, REG_UL, INDIRECT, GEN_REG>
, insn<sz_w, STR("msac"),  OP<0xa080'0120, mac>, FMT_UL0_UL12_0RM_9UL,   REG_UL, REG_UL, INDIR_MASK, GEN_REG>
, insn<sz_w, STR("msac"),  OP<0xa080'0300, mac>, FMT_UL0_UL12_X_0RM_9UL, REG_UL, REG_UL, SF_LEFT, INDIRECT, GEN_REG>
, insn<sz_w, STR("msac"),  OP<0xa080'0320, mac>, FMT_UL0_UL12_X_0RM_9UL, REG_UL, REG_UL, SF_LEFT, INDIR_MASK, GEN_REG>
, insn<sz_w, STR("msac"),  OP<0xa080'0700, mac>, FMT_UL0_UL12_X_0RM_9UL, REG_UL, REG_UL, SF_RIGHT, INDIRECT, GEN_REG>
, insn<sz_w, STR("msac"),  OP<0xa080'0720, mac>, FMT_UL0_UL12_X_0RM_9UL, REG_UL, REG_UL, SF_RIGHT, INDIR_MASK, GEN_REG>

, insn<sz_l, STR("msac"),  OP<0xa080'0900, mac>, FMT_UL0_UL12_0RM_9UL,   GEN_REG, GEN_REG, INDIRECT, GEN_REG>
, insn<sz_l, STR("msac"),  OP<0xa080'0920, mac>, FMT_UL0_UL12_0RM_9UL,   GEN_REG, GEN_REG, INDIR_MASK, GEN_REG>
, insn<sz_l, STR("msac"),  OP<0xa080'0b00, mac>, FMT_UL0_UL12_X_0RM_9UL, GEN_REG, GEN_REG, SF_LEFT, INDIRECT, GEN_REG>
, insn<sz_l, STR("msac"),  OP<0xa080'0b20, mac>, FMT_UL0_UL12_X_0RM_9UL, GEN_REG, GEN_REG, SF_LEFT, INDIR_MASK, GEN_REG>
, insn<sz_l, STR("msac"),  OP<0xa080'0f00, mac>, FMT_UL0_UL12_X_0RM_9UL, GEN_REG, GEN_REG, SF_RIGHT, INDIRECT, GEN_REG>
, insn<sz_l, STR("msac"),  OP<0xa080'0f20, mac>, FMT_UL0_UL12_X_0RM_9UL, GEN_REG, GEN_REG, SF_RIGHT, INDIR_MASK, GEN_REG>
>;

using cf_emac_v = list<list<>
// extended mac
, insn<sz_w, STR("mac"),  OP<0xa000'0000, emac>, FMT_0UL_9UL_7AN,   REG_UL, REG_UL, ACC_N>
, insn<sz_w, STR("mac"),  OP<0xa000'0200, emac>, FMT_0UL_9UL_X_7AN, REG_UL, REG_UL, SF_LEFT, ACC_N>
, insn<sz_w, STR("mac"),  OP<0xa000'0600, emac>, FMT_0UL_9UL_X_7AN, REG_UL, REG_UL, SF_RIGHT, ACC_N>
, insn<sz_l, STR("mac"),  OP<0xa000'0800, emac>, FMT_0UL_9UL_7AN,   GEN_REG, GEN_REG, ACC_N>
, insn<sz_l, STR("mac"),  OP<0xa000'0a00, emac>, FMT_0UL_9UL_X_7AN, GEN_REG, GEN_REG, SF_LEFT, ACC_N>
, insn<sz_l, STR("mac"),  OP<0xa000'0e00, emac>, FMT_0UL_9UL_X_7AN, GEN_REG, GEN_REG, SF_RIGHT, ACC_N>

, insn<sz_w, STR("msac"),  OP<0xa000'0100, emac>, FMT_0UL_9UL_7AN,   REG_UL, REG_UL, ACC_N>
, insn<sz_w, STR("msac"),  OP<0xa000'0300, emac>, FMT_0UL_9UL_X_7AN, REG_UL, REG_UL, SF_LEFT, ACC_N>
, insn<sz_w, STR("msac"),  OP<0xa000'0700, emac>, FMT_0UL_9UL_X_7AN, REG_UL, REG_UL, SF_RIGHT, ACC_N>
, insn<sz_l, STR("msac"),  OP<0xa000'0900, emac>, FMT_0UL_9UL_7AN,   GEN_REG, GEN_REG, ACC_N>
, insn<sz_l, STR("msac"),  OP<0xa000'0b00, emac>, FMT_0UL_9UL_X_7AN, GEN_REG, GEN_REG, SF_LEFT, ACC_N>
, insn<sz_l, STR("msac"),  OP<0xa000'0f00, emac>, FMT_0UL_9UL_X_7AN, GEN_REG, GEN_REG, SF_RIGHT, ACC_N>
   
// Multiply & load
// accumulator_N LSB is inverted! Wow!
, insn<sz_w, STR("macl"),  OP<0xa000'0000, emac>, FMT_UL0_UL12_0RM_9UL_I7AN,   REG_UL, REG_UL, INDIRECT, GEN_REG, ACC_N>
, insn<sz_w, STR("macl"),  OP<0xa000'0020, emac>, FMT_UL0_UL12_0RM_9UL_I7AN,   REG_UL, REG_UL, INDIR_MASK, GEN_REG, ACC_N>
, insn<sz_w, STR("macl"),  OP<0xa000'0200, emac>, FMT_UL0_UL12_X_0RM_9UL_I7AN, REG_UL, REG_UL, SF_LEFT,  INDIRECT, GEN_REG, ACC_N>
, insn<sz_w, STR("macl"),  OP<0xa000'0220, emac>, FMT_UL0_UL12_X_0RM_9UL_I7AN, REG_UL, REG_UL, SF_LEFT,  INDIR_MASK, GEN_REG, ACC_N>
, insn<sz_w, STR("macl"),  OP<0xa000'0600, emac>, FMT_UL0_UL12_X_0RM_9UL_I7AN, REG_UL, REG_UL, SF_RIGHT, INDIRECT, GEN_REG, ACC_N>
, insn<sz_w, STR("macl"),  OP<0xa000'0620, emac>, FMT_UL0_UL12_X_0RM_9UL_I7AN, REG_UL, REG_UL, SF_RIGHT, INDIR_MASK, GEN_REG, ACC_N>

, insn<sz_l, STR("macl"),  OP<0xa000'0800, emac>, FMT_UL0_UL12_0RM_9UL_I7AN,   REG_UL, REG_UL, INDIRECT, GEN_REG, ACC_N>
, insn<sz_l, STR("macl"),  OP<0xa000'0820, emac>, FMT_UL0_UL12_0RM_9UL_I7AN,   REG_UL, REG_UL, INDIR_MASK, GEN_REG, ACC_N>
, insn<sz_l, STR("macl"),  OP<0xa000'0a00, emac>, FMT_UL0_UL12_X_0RM_9UL_I7AN, REG_UL, REG_UL, SF_LEFT,  INDIRECT, GEN_REG, ACC_N>
, insn<sz_l, STR("macl"),  OP<0xa000'0a20, emac>, FMT_UL0_UL12_X_0RM_9UL_I7AN, REG_UL, REG_UL, SF_LEFT,  INDIR_MASK, GEN_REG, ACC_N>
, insn<sz_l, STR("macl"),  OP<0xa000'0e00, emac>, FMT_UL0_UL12_X_0RM_9UL_I7AN, REG_UL, REG_UL, SF_RIGHT, INDIRECT, GEN_REG, ACC_N>
, insn<sz_l, STR("macl"),  OP<0xa000'0e20, emac>, FMT_UL0_UL12_X_0RM_9UL_I7AN, REG_UL, REG_UL, SF_RIGHT, INDIR_MASK, GEN_REG, ACC_N>

, insn<sz_w, STR("msacl"),  OP<0xa000'0100, emac>, FMT_UL0_UL12_0RM_9UL_I7AN,   REG_UL, REG_UL, INDIRECT, GEN_REG, ACC_N>
, insn<sz_w, STR("msacl"),  OP<0xa000'0120, emac>, FMT_UL0_UL12_0RM_9UL_I7AN,   REG_UL, REG_UL, INDIR_MASK, GEN_REG, ACC_N>
, insn<sz_w, STR("msacl"),  OP<0xa000'0300, emac>, FMT_UL0_UL12_X_0RM_9UL_I7AN, REG_UL, REG_UL, SF_LEFT,  INDIRECT, GEN_REG, ACC_N>
, insn<sz_w, STR("msacl"),  OP<0xa000'0320, emac>, FMT_UL0_UL12_X_0RM_9UL_I7AN, REG_UL, REG_UL, SF_LEFT,  INDIR_MASK, GEN_REG, ACC_N>
, insn<sz_w, STR("msacl"),  OP<0xa000'0700, emac>, FMT_UL0_UL12_X_0RM_9UL_I7AN, REG_UL, REG_UL, SF_RIGHT, INDIRECT, GEN_REG, ACC_N>
, insn<sz_w, STR("msacl"),  OP<0xa000'0720, emac>, FMT_UL0_UL12_X_0RM_9UL_I7AN, REG_UL, REG_UL, SF_RIGHT, INDIR_MASK, GEN_REG, ACC_N>

, insn<sz_l, STR("msacl"),  OP<0xa000'0900, emac>, FMT_UL0_UL12_0RM_9UL_I7AN,   REG_UL, REG_UL, INDIRECT, GEN_REG, ACC_N>
, insn<sz_l, STR("msacl"),  OP<0xa000'0920, emac>, FMT_UL0_UL12_0RM_9UL_I7AN,   REG_UL, REG_UL, INDIR_MASK, GEN_REG, ACC_N>
, insn<sz_l, STR("msacl"),  OP<0xa000'0b00, emac>, FMT_UL0_UL12_X_0RM_9UL_I7AN, REG_UL, REG_UL, SF_LEFT,  INDIRECT, GEN_REG, ACC_N>
, insn<sz_l, STR("msacl"),  OP<0xa000'0b20, emac>, FMT_UL0_UL12_X_0RM_9UL_I7AN, REG_UL, REG_UL, SF_LEFT,  INDIR_MASK, GEN_REG, ACC_N>
, insn<sz_l, STR("msacl"),  OP<0xa000'0f00, emac>, FMT_UL0_UL12_X_0RM_9UL_I7AN, REG_UL, REG_UL, SF_RIGHT, INDIRECT, GEN_REG, ACC_N>
, insn<sz_l, STR("msacl"),  OP<0xa000'0f20, emac>, FMT_UL0_UL12_X_0RM_9UL_I7AN, REG_UL, REG_UL, SF_RIGHT, INDIR_MASK, GEN_REG, ACC_N>

// nxp app notes show `l`, but programmers reference doesn't... duplicate entries
, insn<sz_w, STR("mac"),  OP<0xa000'0000, emac>, FMT_UL0_UL12_0RM_9UL_I7AN,   REG_UL, REG_UL, INDIRECT, GEN_REG, ACC_N>
, insn<sz_w, STR("mac"),  OP<0xa000'0020, emac>, FMT_UL0_UL12_0RM_9UL_I7AN,   REG_UL, REG_UL, INDIR_MASK, GEN_REG, ACC_N>
, insn<sz_w, STR("mac"),  OP<0xa000'0200, emac>, FMT_UL0_UL12_X_0RM_9UL_I7AN, REG_UL, REG_UL, SF_LEFT,  INDIRECT, GEN_REG, ACC_N>
, insn<sz_w, STR("mac"),  OP<0xa000'0220, emac>, FMT_UL0_UL12_X_0RM_9UL_I7AN, REG_UL, REG_UL, SF_LEFT,  INDIR_MASK, GEN_REG, ACC_N>
, insn<sz_w, STR("mac"),  OP<0xa000'0600, emac>, FMT_UL0_UL12_X_0RM_9UL_I7AN, REG_UL, REG_UL, SF_RIGHT, INDIRECT, GEN_REG, ACC_N>
, insn<sz_w, STR("mac"),  OP<0xa000'0620, emac>, FMT_UL0_UL12_X_0RM_9UL_I7AN, REG_UL, REG_UL, SF_RIGHT, INDIR_MASK, GEN_REG, ACC_N>

, insn<sz_l, STR("mac"),  OP<0xa000'0800, emac>, FMT_UL0_UL12_0RM_9UL_I7AN,   REG_UL, REG_UL, INDIRECT, GEN_REG, ACC_N>
, insn<sz_l, STR("mac"),  OP<0xa000'0820, emac>, FMT_UL0_UL12_0RM_9UL_I7AN,   REG_UL, REG_UL, INDIR_MASK, GEN_REG, ACC_N>
, insn<sz_l, STR("mac"),  OP<0xa000'0a00, emac>, FMT_UL0_UL12_X_0RM_9UL_I7AN, REG_UL, REG_UL, SF_LEFT,  INDIRECT, GEN_REG, ACC_N>
, insn<sz_l, STR("mac"),  OP<0xa000'0a20, emac>, FMT_UL0_UL12_X_0RM_9UL_I7AN, REG_UL, REG_UL, SF_LEFT,  INDIR_MASK, GEN_REG, ACC_N>
, insn<sz_l, STR("mac"),  OP<0xa000'0e00, emac>, FMT_UL0_UL12_X_0RM_9UL_I7AN, REG_UL, REG_UL, SF_RIGHT, INDIRECT, GEN_REG, ACC_N>
, insn<sz_l, STR("mac"),  OP<0xa000'0e20, emac>, FMT_UL0_UL12_X_0RM_9UL_I7AN, REG_UL, REG_UL, SF_RIGHT, INDIR_MASK, GEN_REG, ACC_N>

, insn<sz_w, STR("msac"),  OP<0xa000'0100, emac>, FMT_UL0_UL12_0RM_9UL_I7AN,   REG_UL, REG_UL, INDIRECT, GEN_REG, ACC_N>
, insn<sz_w, STR("msac"),  OP<0xa000'0120, emac>, FMT_UL0_UL12_0RM_9UL_I7AN,   REG_UL, REG_UL, INDIR_MASK, GEN_REG, ACC_N>
, insn<sz_w, STR("msac"),  OP<0xa000'0300, emac>, FMT_UL0_UL12_X_0RM_9UL_I7AN, REG_UL, REG_UL, SF_LEFT,  INDIRECT, GEN_REG, ACC_N>
, insn<sz_w, STR("msac"),  OP<0xa000'0320, emac>, FMT_UL0_UL12_X_0RM_9UL_I7AN, REG_UL, REG_UL, SF_LEFT,  INDIR_MASK, GEN_REG, ACC_N>
, insn<sz_w, STR("msac"),  OP<0xa000'0700, emac>, FMT_UL0_UL12_X_0RM_9UL_I7AN, REG_UL, REG_UL, SF_RIGHT, INDIRECT, GEN_REG, ACC_N>
, insn<sz_w, STR("msac"),  OP<0xa000'0720, emac>, FMT_UL0_UL12_X_0RM_9UL_I7AN, REG_UL, REG_UL, SF_RIGHT, INDIR_MASK, GEN_REG, ACC_N>

, insn<sz_l, STR("mac"),  OP<0xa000'0900, emac>, FMT_UL0_UL12_0RM_9UL_I7AN,   REG_UL, REG_UL, INDIRECT, GEN_REG, ACC_N>
, insn<sz_l, STR("mac"),  OP<0xa000'0920, emac>, FMT_UL0_UL12_0RM_9UL_I7AN,   REG_UL, REG_UL, INDIR_MASK, GEN_REG, ACC_N>
, insn<sz_l, STR("mac"),  OP<0xa000'0b00, emac>, FMT_UL0_UL12_X_0RM_9UL_I7AN, REG_UL, REG_UL, SF_LEFT,  INDIRECT, GEN_REG, ACC_N>
, insn<sz_l, STR("mac"),  OP<0xa000'0b20, emac>, FMT_UL0_UL12_X_0RM_9UL_I7AN, REG_UL, REG_UL, SF_LEFT,  INDIR_MASK, GEN_REG, ACC_N>
, insn<sz_l, STR("mac"),  OP<0xa000'0f00, emac>, FMT_UL0_UL12_X_0RM_9UL_I7AN, REG_UL, REG_UL, SF_RIGHT, INDIRECT, GEN_REG, ACC_N>
, insn<sz_l, STR("mac"),  OP<0xa000'0f20, emac>, FMT_UL0_UL12_X_0RM_9UL_I7AN, REG_UL, REG_UL, SF_RIGHT, INDIR_MASK, GEN_REG, ACC_N>
>;

using cf_emac_b_v = list<list<>
, insn<sz_w, STR("maaac"),  OP<0xa000'0001, emac_b>, FMT_0UL_9UL_7AN_AN2,   REG_UL, REG_UL, ACC_N, ACC_N>
, insn<sz_w, STR("maaac"),  OP<0xa000'0201, emac_b>, FMT_0UL_9UL_X_7AN_AN2, REG_UL, REG_UL, SF_LEFT, ACC_N, ACC_N>
, insn<sz_w, STR("maaac"),  OP<0xa000'0601, emac_b>, FMT_0UL_9UL_X_7AN_AN2, REG_UL, REG_UL, SF_RIGHT, ACC_N, ACC_N>
, insn<sz_l, STR("maaac"),  OP<0xa000'0801, emac_b>, FMT_0UL_9UL_7AN_AN2,   GEN_REG, GEN_REG, ACC_N, ACC_N>
, insn<sz_l, STR("maaac"),  OP<0xa000'0a01, emac_b>, FMT_0UL_9UL_X_7AN_AN2, GEN_REG, GEN_REG, SF_LEFT, ACC_N, ACC_N>
, insn<sz_l, STR("maaac"),  OP<0xa000'0e01, emac_b>, FMT_0UL_9UL_X_7AN_AN2, GEN_REG, GEN_REG, SF_RIGHT, ACC_N, ACC_N>
    
, insn<sz_w, STR("masac"),  OP<0xa000'0003, emac_b>, FMT_0UL_9UL_7AN_AN2,   REG_UL, REG_UL, ACC_N, ACC_N>
, insn<sz_w, STR("masac"),  OP<0xa000'0203, emac_b>, FMT_0UL_9UL_X_7AN_AN2, REG_UL, REG_UL, SF_LEFT, ACC_N, ACC_N>
, insn<sz_w, STR("masac"),  OP<0xa000'0603, emac_b>, FMT_0UL_9UL_X_7AN_AN2, REG_UL, REG_UL, SF_RIGHT, ACC_N, ACC_N>
, insn<sz_l, STR("masac"),  OP<0xa000'0803, emac_b>, FMT_0UL_9UL_7AN_AN2,   GEN_REG, GEN_REG, ACC_N, ACC_N>
, insn<sz_l, STR("masac"),  OP<0xa000'0a03, emac_b>, FMT_0UL_9UL_X_7AN_AN2, GEN_REG, GEN_REG, SF_LEFT, ACC_N, ACC_N>
, insn<sz_l, STR("masac"),  OP<0xa000'0e03, emac_b>, FMT_0UL_9UL_X_7AN_AN2, GEN_REG, GEN_REG, SF_RIGHT, ACC_N, ACC_N>
    
, insn<sz_w, STR("msaac"),  OP<0xa000'0101, emac_b>, FMT_0UL_9UL_7AN_AN2,   REG_UL, REG_UL, ACC_N, ACC_N>
, insn<sz_w, STR("msaac"),  OP<0xa000'0301, emac_b>, FMT_0UL_9UL_X_7AN_AN2, REG_UL, REG_UL, SF_LEFT, ACC_N, ACC_N>
, insn<sz_w, STR("msaac"),  OP<0xa000'0701, emac_b>, FMT_0UL_9UL_X_7AN_AN2, REG_UL, REG_UL, SF_RIGHT, ACC_N, ACC_N>
, insn<sz_l, STR("msaac"),  OP<0xa000'0901, emac_b>, FMT_0UL_9UL_7AN_AN2,   GEN_REG, GEN_REG, ACC_N, ACC_N>
, insn<sz_l, STR("msaac"),  OP<0xa000'0b01, emac_b>, FMT_0UL_9UL_X_7AN_AN2, GEN_REG, GEN_REG, SF_LEFT, ACC_N, ACC_N>
, insn<sz_l, STR("msaac"),  OP<0xa000'0f01, emac_b>, FMT_0UL_9UL_X_7AN_AN2, GEN_REG, GEN_REG, SF_RIGHT, ACC_N, ACC_N>
    
, insn<sz_w, STR("mssac"),  OP<0xa000'0101, emac_b>, FMT_0UL_9UL_7AN_AN2,   REG_UL, REG_UL, ACC_N, ACC_N>
, insn<sz_w, STR("mssac"),  OP<0xa000'0301, emac_b>, FMT_0UL_9UL_X_7AN_AN2, REG_UL, REG_UL, SF_LEFT, ACC_N, ACC_N>
, insn<sz_w, STR("mssac"),  OP<0xa000'0701, emac_b>, FMT_0UL_9UL_X_7AN_AN2, REG_UL, REG_UL, SF_RIGHT, ACC_N, ACC_N>
, insn<sz_l, STR("mssac"),  OP<0xa000'0901, emac_b>, FMT_0UL_9UL_7AN_AN2,   GEN_REG, GEN_REG, ACC_N, ACC_N>
, insn<sz_l, STR("mssac"),  OP<0xa000'0b01, emac_b>, FMT_0UL_9UL_X_7AN_AN2, GEN_REG, GEN_REG, SF_LEFT, ACC_N, ACC_N>
, insn<sz_l, STR("mssac"),  OP<0xa000'0f01, emac_b>, FMT_0UL_9UL_X_7AN_AN2, GEN_REG, GEN_REG, SF_RIGHT, ACC_N, ACC_N>
>;

using cf_supv_v = list<list<>
// re-add core insns otherwise excluded
, insn<sz_vb, STR("tas"),  OP<0x4ac0, isa_c>, FMT_0RM, DATA_ALTER>
, insn<sz_vl, STR("extb"), OP<0x49c0, coldfire>, FMT_0D, DATA_REG>

, insn<sz_l, STR("byterev"), OP<0x2c0, isa_c>, FMT_0D, DATA_REG>

, insn<sz_l, STR("mov3q"),  OP<0xa140, isa_b>, FMT_9QM_0RM, Q_MOV3Q, ALTERABLE>
, insn<sz_w, STR("mvs"),    OP<0x7140, isa_b>, FMT_0RM_9D, GEN, DATA_REG>
, insn<sz_b, STR("mvs"),    OP<0x7100, isa_b>, FMT_0RM_9D, GEN, DATA_REG>
, insn<sz_w, STR("mvz"),    OP<0x71c0, isa_b>, FMT_0RM_9D, GEN, DATA_REG>
, insn<sz_b, STR("mvz"),    OP<0x7180, isa_b>, FMT_0RM_9D, GEN, DATA_REG>
, insn<sz_l, STR("sats"),   OP<0x4c80, isa_b>, FMT_0D, DATA_REG>
, insn<sz_w, STR("tpf"),    OP<0x51fa, coldfire>, void, IMMED> 
, insn<sz_l, STR("tpf"),    OP<0x51fb, coldfire>, void, IMMED> 
, insn<sz_v, STR("tpf"),    OP<0x51fc, coldfire>>
    
, insn<sz_v, STR("pulse"),   OP<0x4acc, coldfire>>

// XXX REMS/REMU not supported by MCF5202/5204/5206
, insn<sz_l, STR("rems"),    OP<0x4c40'0800, isa_a>, FMT_0RM_PAIR, DATA_REG, PAIR>
, insn<sz_l, STR("rems"),    OP<0x4c40'0800, isa_a>, FMT_0RM_PAIR, ADDR_INDIR, PAIR> 
, insn<sz_l, STR("remu"),    OP<0x4c40'0000, isa_a>, FMT_0RM_PAIR, DATA_REG, PAIR>
, insn<sz_l, STR("remu"),    OP<0x4c40'0000, isa_a>, FMT_0RM_PAIR, ADDR_INDIR, PAIR> 

, insn<sz_v, STR("intouch"), OP<0xf428,      isa_b>, FMT_0S, ADDR_INDIR>
, insn<sz_w, STR("strldsr"), OP<0x40e7'46fc, isa_c>, void,  IMMED>     // immed word

// DEBUG cp
, insn<sz_l, STR("wdebug"),  OP<0xfbc0'0003, isa_a>, FMT_0S, ADDR_INDIR>
, insn<sz_l, STR("wdebug"),  OP<0xfbc0'0003, isa_a>, FMT_0A, ADDR_DISP> 
, insn<sz_lwb, STR("wddata"), OP<0xfb00, isa_a>, FMT_0RM, MEM_ALTER>
    
>; 

using cf_insn_v = list<list<>
                , cf_mac_moves_v
                , cf_mac_v
                , cf_emac_v
                , cf_emac_b_v
                , cf_supv_v
                >;

#undef STR
}

namespace kas::m68k::opc
{
    template <> struct m68k_insn_defn_list<OP_COLDFIRE> : ns_coldfire::cf_insn_v {};
}

#endif
