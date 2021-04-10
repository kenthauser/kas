#ifndef KBFD_KBFD_OBJECT_IMPL_H
#define KBFD_KBFD_OBJECT_IMPL_H

#include "kbfd_object.h"
#include "kbfd_section_sym.h"
#include "kbfd_target_format.h"

#include "target/z80_aout.h"

namespace kbfd
{

kbfd_object::kbfd_object(kbfd_target_format const& fmt)
    : fmt(fmt), cvt(fmt.cvt), swap(cvt.swap), e_hdr(fmt.init_header())
    {}

std::size_t kbfd_object::add_section_p(kbfd_section *p)
{
    section_ptrs.emplace_back(p);
    return section_ptrs.size();
}

// optional method. reserve std::vector space
void kbfd_object::reserve_sections(unsigned n_sections)
{
    section_ptrs.reserve(n_sections);
}

const char *kbfd_object::write(std::ostream& os)
{
    fmt.write(*this, os);
    return {};
}

const char *kbfd_object::sym_name(kbfd_sym const& sym) const 
{
    if (symtab_p)
        return symtab_p->sym_name(sym);
    return "";
}

const char *kbfd_object::sym_name(kbfd_sym_index_t n_sym) const 
{
    if (symtab_p)
        return symtab_p->sym_name(n_sym);
    return "";
}

kbfd_target_reloc const *kbfd_object::get_reloc(kbfd_reloc const& reloc) const
{
    return fmt.lookup(reloc);
}

}
#endif

