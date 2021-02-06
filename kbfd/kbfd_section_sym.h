#ifndef KBFD_KBFD_SECTION_SYM_H
#define KBFD_KBFD_SECTION_SYM_H


#include "kbfd_section.h"

namespace kbfd
{

// create a string section
struct ks_string : kbfd_section
{
    ks_string(kbfd_object& obj, std::string name)
        : kbfd_section(obj, SHT_STRTAB, name)
    {
        kbfd_section::put("", 1);       // NUL first string
    }

    // delete raw access
    void put(void const *, std::size_t) = delete;
    
    auto put(std::string const& s)
    {
        auto position = kbfd_section::position();
        kbfd_section::put(s.data(), s.size() + 1);
        return position;
    }

    auto put(const char *p)
    {
        auto position = kbfd_section::position();
        kbfd_section::put(p, std::strlen(p) + 1);
        return position;
    }
};

// create a symbol section
// NB: hold copy of symbols in host format,
// convert to target format on writing,
// or to host format on reading
struct ks_symbol : kbfd_section
{
    // default initializer -- system symbol table
    ks_symbol(kbfd_object& obj) : ks_symbol(obj, ".symtab", ".strtab") {}

    // actual initializer
    ks_symbol(kbfd_object& obj, std::string tab_name, std::string str_name);
    
    // host objects use "add", not "put"
    void put(void const *, std::size_t) = delete;

    // generate target symbol table from host table
    // do `static_cast` in object, not calling location
    static void gen_target_data(kbfd_object& obj, kbfd_section *s)
    {
        static_cast<ks_symbol *>(s)->do_gen_target(obj);
    }

    // make room for expected "entries"
    void reserve(Elf64_Word size)
    {
        host_table.reserve(size);
    }
    
    // add symbol to host symbol table: Elf64_Sym
    Elf64_Word add(Elf64_Sym const& new_sym, std::string const& name = {});

    const char *sym_name(Elf64_Sym const& sym) const
    {
        return sym_string.begin() + sym.st_name;
    }

    const char *sym_name(Elf64_Word index) const
    {
        return sym_name(get(index));
    }

    Elf64_Sym const& get(Elf64_Word index) const
    {
        return host_table[index];
    }

    // a.out needs direct access to string table
    auto& strtab() const { return sym_string; }

private:
    // convert "host" symbol table to "target" object
    void do_gen_target(kbfd_object& obj);
    
    std::vector<Elf64_Sym> host_table;
    ks_string sym_string;        // auxillary section for symbol names 
};
   
}
#endif
