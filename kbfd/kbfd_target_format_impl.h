#ifndef KBFD_KBFD_TARGET_FORMAT_IMPL_H
#define KBFD_KBFD_TARGET_FORMAT_IMPL_H

#include "kbfd_target_format.h"
#include "kbfd_section_defns.h"
#include <unordered_map>
#include <vector>
#include <typeindex>

namespace kbfd
{
//  default `section_defns`
auto kbfd_target_format::get_section_defns() const
    -> kbfd_target_sections const&
{
    static const kbfd_target_sections defn;
    return defn;
}

auto kbfd_target_format::lookup(kbfd_reloc const& reloc) const
    -> kbfd_target_reloc const *
{
    std::cout << "kbfd_target_format::lookup: reloc = " << reloc << std::endl;
    
    // use `unordered_map`s for `derived type` & `reloc` lookup
    using tgt_reloc_map_t = std::unordered_map<kbfd_reloc::key_t
                                             , kbfd_target_reloc const *>;
    using map_of_maps     = std::unordered_map<std::type_index, tgt_reloc_map_t *>;
    
    // find proper map based on derived type
    static map_of_maps mom;
    auto& map_p = mom[std::type_index(typeid(*this))];

#ifdef XXX
    // initialize reloc_map on first reference
    if (!map_p)
        map_p = new tgt_reloc_map_t(relocs, relocs + num_relocs);
#else
    if (!map_p)
    {
        // NB: need to update `relocs` array for direct initialzation
        map_p = new tgt_reloc_map_t;
        auto p = relocs;
        for (auto i = 0; i < num_relocs; ++i, ++p)
            map_p->emplace(p->reloc.key(), p);
    }
#endif
    // find `kbfd__target_reloc` which corresponds to `reloc` (if any)
    auto result = map_p->find(reloc.key());
    if (result != map_p->end())
        return result->second;
    return {};
}

auto kbfd_target_format::get_p(target_reloc_index_t index) const
    -> kbfd_target_reloc const *
{
    std::cout << "kbfd_target_format::get: index = " << +index << std::endl;
    
    using tgt_index_map_t = std::vector<kbfd_target_reloc const *>;
    using map_of_maps     = std::unordered_map<std::type_index, tgt_index_map_t *>;
    
    // find proper map based on derived type
    static map_of_maps mom;
    auto& map_p = mom[std::type_index(typeid(*this))];

    // initialize index_map on first reference
    if (!map_p)
    {
        // reserve "max_relocs". (NB: need to calculate "max_relocs")
        map_p = new tgt_index_map_t(max_relocs + 1);
        auto p = relocs;
        for (auto i = 0; i < num_relocs; ++i, ++p)
            (*map_p)[p->num] = p;
    }

    // lookup `index` if in range. 
    if (index <= max_relocs)
        return (*map_p)[index];
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

