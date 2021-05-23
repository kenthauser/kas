#ifndef KAS_KBFD_TARGET_ARM_H
#define KAS_KBFD_TARGET_ARM_H

#include "kbfd/kbfd.h"        // boilerplate
#include "arm_reloc_ops.h"

namespace kbfd
{

// boilerplate for target arm
namespace arm
{
    template <typename tag = void> struct arm_formats_v : meta::id<void> {};

    // default format is `elf`
    //template <> struct arm_formats_v<void> : FORMAT_ELF {};
    
    // relocation "target"
    enum arm_reloc_target { T_DATA, T_ARM, T_T16, T_T32 };
}

// retrieve all defined target formats
template <> struct kbfd_targets_v<TARGET_ARM>
                    { using type = meta::quote_trait<arm::arm_formats_v>; };
}
#endif
