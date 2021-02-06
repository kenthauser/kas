#ifndef KBFD_KBFD_H
#define KBFD_KBFD_H

//#include "kas_relocations.h"        // declare relocation infrastructure

#include "kbfd_object.h"
#include "kas/kas_string.h"         // define string types
#include "kas/defn_utils.h"         // using `all_defns` infrastructure

#include <meta/meta.hpp>            // metaprogramming types

namespace kbfd
{
using namespace kas;                // XXX temp: after namespace `kbfd` xform


// declare the object code target architectures
using TARGET_M68K   = KAS_STRING("M68K");
using TARGET_Z80    = KAS_STRING("Z80");
using TARGET_I386   = KAS_STRING("I386");
using TARGET_ARM    = KAS_STRING("ARM");

using target_tags   = meta::list<TARGET_M68K
                               , TARGET_Z80
                               , TARGET_I386
                               , TARGET_ARM
                               >;

// declare the target object code formats
using FORMAT_ELF    = KAS_STRING("ELF");
using FORMAT_AOUT   = KAS_STRING("AOUT");
using FORMAT_COFF   = KAS_STRING("COFF");

using format_tags   = meta::list<FORMAT_ELF
                               , FORMAT_AOUT
                               , FORMAT_COFF
                               >;

// specialize to define implemented formats
template <typename tag = void> struct kas_formats_v : meta::list<> {};
template <typename tag = void> struct kas_targets_v : meta::list<> {};


// retrieve `kbfd_format` object for specified target/format
kbfd_format const *get_obj_format(const char *target = {}
                                , const char *format = {});
}


#endif
