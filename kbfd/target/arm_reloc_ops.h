#ifndef KBFD_TARGET_ARM_RELOC_OPS_H
#define KBFD_TARGET_ARM_RELOC_OPS_H

#include "../kbfd_reloc_ops.h"

namespace kbfd
{

// relocation operations used by ARM architecture
using ARM_REL_MOVW   = KAS_STRING("ARM_REL_MOVW");
using ARM_REL_MOVT   = KAS_STRING("ARM_REL_MOVT");
using ARM_REL_V4BX   = KAS_STRING("ARM_REL_V4BX");
using ARM_REL_OFF12  = KAS_STRING("ARM_REL_OFF12");
using ARM_REL_OFF24  = KAS_STRING("ARM_REL_OFF24");

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
    struct arm_rel_off24 : k_rel_add_t, reloc_op_subfield<24, 0> {};
    struct arm_rel_sel12;
}

// add operations to `kbfd` list of ops
template <> struct reloc_ops_v<TARGET_ARM> : meta::list<
          KBFD_ACTION<ARM_REL_MOVW      , arm::arm_rel_movw>
        , KBFD_ACTION<ARM_REL_MOVT      , arm::arm_rel_movw>
        , KBFD_ACTION<ARM_REL_OFF12     , arm::arm_rel_sel12>
        , KBFD_ACTION<ARM_REL_OFF24     , arm::arm_rel_off24>
        , KBFD_ACTION<ARM_REL_V4BX>
        > {};
}


#endif
