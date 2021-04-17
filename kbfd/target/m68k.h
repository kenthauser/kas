#ifndef KAS_KBFD_TARGET_M68K_H
#define KAS_KBFD_TARGET_M68K_H

#include "kbfd/kbfd.h"        // boilerplate

namespace kbfd
{

// boilerplate for target m68k
namespace m68k
{
    template <typename tag = void> struct m68k_formats_v : meta::id<void> {};

    // default format is `elf`
    //template <> struct m68k_formats_v<void> : FORMAT_ELF {};
}

// retrieve all defined target formats
template <> struct kbfd_targets_v<TARGET_M68K>
                    { using type = meta::quote_trait<m68k::m68k_formats_v>; };
}
#endif
