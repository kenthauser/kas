#ifndef KBFD_KBFD_FORMAT_IMPL_H
#define KBFD_KBFD_FORMAT_IMPL_H

#include "kbfd_format.h"
#include <map>


namespace kbfd
{
auto kbfd_format::lookup(kbfd_reloc const& reloc) const
    -> kbfd_target_reloc const *
{
    std::cout << "kbfd_format::lookup: reloc = " << reloc << std::endl;
    return {};
}

auto kbfd_format::get(target_reloc_index_t index) const
    -> kbfd_target_reloc const *
{
    std::cout << "kbfd_format::get: index = " << +index << std::endl;
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

