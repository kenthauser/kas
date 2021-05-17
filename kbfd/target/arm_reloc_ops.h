#ifndef KBFD_TARGET_ARM_RELOC_OPS_H
#define KBFD_TARGET_ARM_RELOC_OPS_H

#include "../kbfd_reloc_ops.h"

namespace kbfd
{

// relocations used by ARM architecture
using ARM_REL_MOVW   = KAS_STRING("ARM_REL_MOVW");
using ARM_REL_MOVT   = KAS_STRING("ARM_REL_MOVT");

namespace arm
{
    struct arm_rel_movw : k_rel_add_t, reloc_op_subfield<4, 12> {};
        
    struct arm_rel_movt : k_rel_add_t, reloc_op_subfield<4, 12> {};

}

template <> struct reloc_ops_v<TARGET_ARM> : meta::list<
          KBFD_ACTION<ARM_REL_MOVW      , arm::arm_rel_movw>
        , KBFD_ACTION<ARM_REL_MOVT      , arm::arm_rel_movw>
        > {};
}


#endif
