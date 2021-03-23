#ifndef KAS_CORE_CORE_SECTION_IMPL_H
#define KAS_CORE_CORE_SECTION_IMPL_H

#include "core_section.h"


namespace kas::core
{

void core_section::init(kbfd::kbfd_object const& obj)
{
    defn_p = &obj.fmt.get_section_defns();
    // allocate "default" sections
    // NB: put here so they are numbered 1, 2, 3
    // NB: this normalizes assembly listings
    // 
    // NB: just a convention. I can find no requirement.
    // NB: this can be safely removed. (at risk of modifying listings)

    core_section::get(".text");
    core_section::get(".data");
    core_section::get(".bss");
}

core_section& core_section::get_initial()
{
    return get(".text");
}

core_section& core_section::get_lcomm()
{
    return get(".bss");
}




}

#endif
