#ifndef KBFD_TARGET_ARM_RELOC_OPS_H
#define KBFD_TARGET_ARM_RELOC_OPS_H

// access generic relocations
#include "kbfd/kbfd_reloc_ops.h"

namespace kbfd
{

// static relocation operations used by A32 architecture
using ARM_REL_NO_TFUNC = KAS_STRING("ARM_REL_NO_TFUNC");
using ARM_REL_MOVW     = KAS_STRING("ARM_REL_MOVW");
using ARM_REL_MOVT     = KAS_STRING("ARM_REL_MOVT");
using ARM_REL_V4BX     = KAS_STRING("ARM_REL_V4BX");
using ARM_REL_A32ALU   = KAS_STRING("ARM_REL_A32ALU");  // calculate immediate value
using ARM_REL_A32LDR   = KAS_STRING("ARM_REL_A32LDR");  // ldr/str
using ARM_REL_A32LDRS  = KAS_STRING("ARM_REL_A32LDRS"); // ldr/str H/SH/SB/D
using ARM_REL_A32LDC   = KAS_STRING("ARM_REL_A32LDC");  // co-prossessor
using ARM_REL_A32JUMP  = KAS_STRING("ARM_REL_A32JUMP");
using ARM_REL_IMMED12  = KAS_STRING("ARM_REL_IMMED12");

// static relocation operations used by THUMB_16 architecture
using ARM_REL_ABS5     = KAS_STRING("ARM_REL_ABS5");
using ARM_REL_PC8      = KAS_STRING("ARM_REL_PC8");
using ARM_REL_JUMP6    = KAS_STRING("ARM_REL_JUMP6");
using ARM_REL_JUMP8    = KAS_STRING("ARM_REL_JUMP8");
using ARM_REL_JUMP11   = KAS_STRING("ARM_REL_JUMP11");
using ARM_REL_THB_CALL = KAS_STRING("ARM_REL_THB_CALL");

namespace arm
{
// declare ARM relocations
    using arm_reloc_ops = meta::list<
          KBFD_ACTION<ARM_REL_NO_TFUNC  , struct arm_rel_no_tfunc>
        , KBFD_ACTION<ARM_REL_MOVW      , struct arm_rel_movw>
        , KBFD_ACTION<ARM_REL_MOVT      , struct arm_rel_movw>
        , KBFD_ACTION<ARM_REL_A32LDR    , struct arm_rel_a32ldr>
        , KBFD_ACTION<ARM_REL_A32LDRS   , struct arm_rel_a32ldrs>
        , KBFD_ACTION<ARM_REL_A32LDC    , struct arm_rel_a32ldc>
        , KBFD_ACTION<ARM_REL_A32JUMP   , struct arm_rel_a32jump>
        , KBFD_ACTION<ARM_REL_IMMED12   , struct arm_rel_immed12>
        , KBFD_ACTION<ARM_REL_A32ALU    , struct arm_rel_a32alu>
        , KBFD_ACTION<ARM_REL_V4BX      , struct arm_rel_v4bx>
        , KBFD_ACTION<ARM_REL_ABS5      , struct arm_rel_abs5>
        , KBFD_ACTION<ARM_REL_PC8       , struct arm_rel_pc8>
        , KBFD_ACTION<ARM_REL_JUMP6     , struct arm_rel_jump6>
        , KBFD_ACTION<ARM_REL_JUMP8     , struct arm_rel_jump8>
        , KBFD_ACTION<ARM_REL_JUMP11    , struct arm_rel_jump11>
        , KBFD_ACTION<ARM_REL_THB_CALL  , struct arm_rel_thb_call>
        >;
}

// add operations to `kbfd` list of ops
template <> struct reloc_ops_v<TARGET_ARM> : arm::arm_reloc_ops {};
}


#endif
