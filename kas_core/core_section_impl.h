#ifndef KAS_CORE_CORE_SECTION_IMPL_H
#define KAS_CORE_CORE_SECTION_IMPL_H

#include "core_section.h"
#include "kbfd/kbfd_target_format.h"

namespace kas::core
{

void core_section::init(kbfd::kbfd_object const& obj)
{
    // allocate "default" sections
    // NB: normally .text, .data, .bss
    // NB: put here so they are numbered 1, 2, 3
    // NB: this normalizes assembly listings
    // 
    // NB: just a convention. I can find no requirement.
    // NB: this can be safely removed. (at risk of modifying listings)
    
    // save pointer to `kbfd` section definitions
    defn_p = &obj.fmt.get_section_defns();

    // allocate sections specified by `kbfd` list (if any)
    auto [it, end] = defn_p->get_initial_sections();
    while (it != end)
        get(*it++);
}

core_section& core_section::get_initial()
{
    return get(*defn_p->get_initial());
}

core_section& core_section::get_lcomm()
{
    return get(*defn_p->get_local_common());
}

core_section& core_section::get(kbfd::kbfd_section_defn const& d)
{
    return get(d.name, d.sh_type, d.sh_flags);
}

core_segment& core_section::segment(unsigned subsection) const
{
    auto& seg_p = segments[subsection];
    if (!seg_p)
        seg_p = &core_segment::add(*this, subsection);
    return *seg_p;
}

std::size_t core_section::size() const
{
    std::size_t size{};
    for (auto& seg : segments)
        size += seg.second->size()();
    return size;
}

template <typename OS>
void core_section::dump(OS& os)
{
    auto print_section = [&](auto const& s)
    {
        // print section # in decimal, everything else in hex.
        os << std::dec;
        os <<         std::right << std::setw(4)  << s.index() - 1;
        os << std::hex;
        os << ": " << std::left  << std::setw(20) << s.sh_name;
        os << ": " << std::setw(2) << std::right  << s.sh_type;
        os << " "  << std::setw(8) << std::right  << s.sh_flags;
        //os << " "  << std::setw(2) << std::right  << s.sh_entsize;
        os << " "  << std::setw(2) << std::right  << s.kas_linkage;
        os << " "  << std::setw(20) << std::left  << s.kas_group;
        os << std::endl;
    };

    os << "sections:" << std::endl;
    for_each(print_section);
    os << std::endl;
}


}

#endif
