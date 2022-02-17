#ifndef KBFD_TARGET_ARM_RELOC_OPS_H
#define KBFD_TARGET_ARM_RELOC_OPS_H

// access generic relocations
#include "kbfd/kbfd_reloc_ops.h"

namespace kbfd
{

// static relocation operations used by A32 architecture
using ARM_REL_MOVW     = KAS_STRING("ARM_REL_MOVW");
using ARM_REL_MOVT     = KAS_STRING("ARM_REL_MOVT");
using ARM_REL_V4BX     = KAS_STRING("ARM_REL_V4BX");
using ARM_REL_SOFF12   = KAS_STRING("ARM_REL_SOFF12");
using ARM_REL_OFF24    = KAS_STRING("ARM_REL_OFF24");
using ARM_REL_IMMED12  = KAS_STRING("ARM_REL_IMMED12");
using ARM_REL_ADDSUB   = KAS_STRING("ARM_REL_ADDSUB");

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
          KBFD_ACTION<ARM_REL_MOVW      , struct arm_rel_movw>
        , KBFD_ACTION<ARM_REL_MOVT      , struct arm_rel_movw>
        , KBFD_ACTION<ARM_REL_SOFF12    , struct arm_rel_soff12>
        , KBFD_ACTION<ARM_REL_OFF24     , struct arm_rel_off24>
        , KBFD_ACTION<ARM_REL_IMMED12   , struct arm_rel_immed12>
        , KBFD_ACTION<ARM_REL_ADDSUB    , struct arm_rel_addsub>
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
