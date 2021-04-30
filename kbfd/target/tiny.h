#ifndef KAS_KBFD_TARGET_TINY_H
#define KAS_KBFD_TARGET_TINY_H

#include "kbfd/kbfd.h"        // boilerplate

namespace kbfd
{

// boilerplate for target tiny
namespace tiny
{
    template <typename tag = void> struct tiny_formats_v : meta::id<void> {};

    // default format is `a.out`
    //template <> struct tiny_formats_v<void> : FORMAT_AOUT {};
}

// retrieve all defined target formats
template <> struct kbfd_targets_v<TARGET_TINY>
                    { using type = meta::quote_trait<tiny::tiny_formats_v>; };
}

#endif
