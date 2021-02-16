#ifndef KBFD_KBFD_TARGET_FORMAT_IMPL_H
#define KBFD_KBFD_TARGET_FORMAT_IMPL_H

#include "kbfd_target_format.h"
#include <map>


namespace kbfd
{
auto kbfd_target_format::lookup(kbfd_reloc const& reloc) const
    -> kbfd_target_reloc const *
{
    using tgt_reloc_map_t = std::map<kbfd_reloc::key_t
                                   , kbfd_target_reloc const *>;
#if 0
    static tgt_reloc_map_t *map_p;

    if (!map_p)
    {
        map_p = new(decltype(*map_p));
        auto p = relocs;
        for (auto i = 0; i < num_relocs; ++i, ++p)
        {
        }
    }
#endif
    std::cout << "kbfd_target_format::lookup: reloc = " << reloc << std::endl;

    return {};
}

auto kbfd_target_format::get(target_reloc_index_t index) const
    -> kbfd_target_reloc const *
{
    std::cout << "kbfd_target_format::get: index = " << +index << std::endl;
    return {};
}


std::ostream& operator<<(std::ostream& os, kbfd_reloc const& reloc)
{
    os << "[" << reloc.action.name();
    os << ", bits = " << +reloc.bits;
    os << ", flags = "; 
    if (reloc.flags & reloc.RFLAGS_PC_REL)
        os << "PC_REL "; 
    os << "]";
    return os;
}
}

#endif

