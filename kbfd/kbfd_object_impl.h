#ifndef KBFD_KBFD_OBJECT_IMPL_H
#define KBFD_KBFD_OBJECT_IMPL_H

#include "kbfd_object.h"
#include "kbfd_section_sym.h"
#include "kbfd_format.h"



namespace kbfd
{
kbfd_object::kbfd_object(kbfd_format const& fmt)
    : fmt(fmt), cvt(fmt.cvt), swap(cvt.swap)
{
    e_hdr = fmt.init_header();
}

const char *kbfd_object::write(std::ostream& os)
{
    fmt.write(*this, os);
    return {};
}

const char *kbfd_object::sym_name(Elf64_Sym const& sym) const 
{
    if (symtab_p)
        return symtab_p->sym_name(sym);
    return "";
}

const char *kbfd_object::sym_name(Elf64_Word n_sym) const 
{
    if (symtab_p)
        return symtab_p->sym_name(n_sym);
    return "";
}


kbfd_format const *get_obj_format(const char *target, const char *format)
{
    static m68k::m68k_elf obj;
    return &obj;
}

}
#endif

