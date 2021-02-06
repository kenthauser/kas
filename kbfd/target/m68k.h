#ifndef KAS_KBFD_TARGET_M68K_H
#define KAS_KBFD_TARGET_M68K_H

#include "kbfd/kbfd.h"        // boilerplate
//#include "m68k_elf.h"       // suppported target

namespace kbfd
{

namespace m68k
{
    // specialize `elf.h` template with `TARGET_M68K`
    template <typename tag = void> struct m68k_formats_v : meta::list<> {};

    // XXX need to name "values" for machine type defns
    // ie target types, etc, that will be passed to configure backends
    // XXX assume back-end which uses them understands semantics
}

// retrieve all defined target formats
template <> struct kas_targets_v<TARGET_M68K>
                    : meta::quote<m68k::m68k_formats_v> {};
        
}


#endif
