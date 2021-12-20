#ifndef KBFD_TARGET_ARM_RELOC_OPS_H
#define KBFD_TARGET_ARM_RELOC_OPS_H

#include "../kbfd_reloc_ops.h"

namespace kbfd
{

// static relocation operations used by ARM7 architecture
using ARM_REL_MOVW   = KAS_STRING("ARM_REL_MOVW");
using ARM_REL_MOVT   = KAS_STRING("ARM_REL_MOVT");
using ARM_REL_V4BX   = KAS_STRING("ARM_REL_V4BX");
using ARM_REL_SOFF12 = KAS_STRING("ARM_REL_SOFF12");
using ARM_REL_OFF24  = KAS_STRING("ARM_REL_OFF24");
using ARM_REL_ADDSUB = KAS_STRING("ARM_REL_ADDSUB");

// static relocation operations used by THUMB_16 architecture
using ARM_REL_ABS5   = KAS_STRING("ARM_REL_ABS5");
using ARM_REL_PC8    = KAS_STRING("ARM_REL_PC8");
using ARM_REL_JUMP6  = KAS_STRING("ARM_REL_JUMP6");
using ARM_REL_JUMP8  = KAS_STRING("ARM_REL_JUMP8");
using ARM_REL_JUMP11 = KAS_STRING("ARM_REL_JUMP11");
using ARM_REL_THB_CALL = KAS_STRING("ARM_REL_THB_CALL");

namespace arm
{
    // support routines for alu modified immediate constants
    // returns: { encoded_value, residual }
    std::pair<uint16_t, uint32_t> encode_alu_immed(uint32_t);
    // returns: decoded value
    uint16_t                      decode_alu_immed(uint16_t);
    
    // XXX arm_rel_movw/movt are incorrect: 
    struct arm_rel_movw : k_rel_add_t, reloc_op_subfield<4, 12> {};
    struct arm_rel_movt : k_rel_add_t, reloc_op_subfield<4, 12> {};
    struct arm_rel_off24;
    struct arm_rel_soff12;
    struct arm_rel_addsub;
    struct arm_rel_v4bx;

    // thumb
    struct arm_rel_abs5;
    struct arm_rel_pc8;
    struct arm_rel_jump6;
    struct arm_rel_jump8;
    struct arm_rel_jump11;
    struct arm_rel_thb_call;

}

// add operations to `kbfd` list of ops
template <> struct reloc_ops_v<TARGET_ARM> : meta::list<
          KBFD_ACTION<ARM_REL_MOVW      , arm::arm_rel_movw>
        , KBFD_ACTION<ARM_REL_MOVT      , arm::arm_rel_movw>
        , KBFD_ACTION<ARM_REL_SOFF12    , arm::arm_rel_soff12>
        , KBFD_ACTION<ARM_REL_OFF24     , arm::arm_rel_off24>
        , KBFD_ACTION<ARM_REL_ADDSUB    , arm::arm_rel_addsub>
        , KBFD_ACTION<ARM_REL_V4BX      , arm::arm_rel_v4bx>
        , KBFD_ACTION<ARM_REL_ABS5      , arm::arm_rel_abs5>
        , KBFD_ACTION<ARM_REL_PC8       , arm::arm_rel_pc8>
        , KBFD_ACTION<ARM_REL_JUMP6     , arm::arm_rel_jump6>
        , KBFD_ACTION<ARM_REL_JUMP8     , arm::arm_rel_jump8>
        , KBFD_ACTION<ARM_REL_JUMP11    , arm::arm_rel_jump11>
        , KBFD_ACTION<ARM_REL_THB_CALL  , arm::arm_rel_thb_call>
        > {};
}


#endif
