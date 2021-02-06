#ifndef KBFD_KBFD_FORMAT_IMPL_H
#define KBFD_KBFD_FORMAT_IMPL_H

#include "kbfd_format.h"
#include <map>


namespace kbfd
{
auto kbfd_format::lookup(kbfd_reloc const& reloc) const
    -> kbfd_target_reloc const *
{
    return {};
}

auto kbfd_format::get(target_reloc_index_t index) const
    -> kbfd_target_reloc const *
{
    return {};
}
}

#endif

