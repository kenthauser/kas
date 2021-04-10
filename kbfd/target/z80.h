#ifndef KAS_KBFD_TARGET_Z80_H
#define KAS_KBFD_TARGET_Z80_H

#include "kbfd/kbfd.h"        // boilerplate

namespace kbfd
{

// boilerplate for target z80
namespace z80
{
    template <typename tag = void> struct z80_formats_v : meta::list<> {};

    // default format is `a.out`
    template <> struct z80_formats_v<void> : FORMAT_AOUT {};
}

// retrieve all defined target formats
template <> struct kbfd_targets_v<TARGET_Z80>
                    : meta::quote<z80::z80_formats_v> {};

}

#endif
