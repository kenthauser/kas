#ifndef KBFD_KBFD_SECTION_SYM_IMPL_H
#define KBFD_KBFD_SECTION_SYM_IMPL_H

//
// implement non-trivial methods
//

#include "kbfd_section_sym.h"
#include "kbfd_convert.h"

namespace kbfd
{

// actual initializer
ks_symbol::ks_symbol(kbfd_object& obj, std::string tab_name, std::string str_name)
    : sym_string (obj, str_name)
    , kbfd_section(obj, SHT_SYMTAB, tab_name, sizeof(Elf_Sym))
{
    s_header.sh_link    = sym_string.index;
    s_header.sh_entsize = obj.cvt.tgt_size<Elf64_Sym>();

    // initial symbol is zero entry
    host_table.emplace_back();
}

// add symbol to host symbol table: Elf64_Sym
Elf64_Word ks_symbol::add(Elf64_Sym const& new_sym, std::string const& name)
{
    // symbol number is index value (zero-based)
    auto sym_num = host_table.size();

    // allocate symbol
    Elf64_Sym& sym = host_table.emplace_back(new_sym);

    if (!name.empty())
        sym.st_name = sym_string.put(name);

    // set `sh_info` to one past last local symbol
    auto st_bind = ELF64_ST_BIND(sym.st_info);
    if (st_bind == STB_LOCAL)
        s_header.sh_info = sym_num + 1;
    
    return sym_num;
}

// convert "host" object to "target" object
void ks_symbol::do_gen_target(kbfd_object& obj)
{
    // XXX if passthru just modify pointers...
    auto cnt = host_table.size();           // get entry count...
    set_size(cnt * s_header.sh_entsize);    // ... and allocate memory
    
    // convert host -> target
    for (auto& s : host_table)
        kbfd_section::put(obj.cvt(s));
}

}
#endif
