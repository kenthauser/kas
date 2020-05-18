#ifndef KAS_ELF_ELF_SECTION_SYM_H
#define KAS_ELF_ELF_SECTION_SYM_H


#include "elf_section.h"

namespace kas::elf
{

// create a string section
struct es_string : elf_section
{
    es_string(elf_object& obj, std::string name)
        : elf_section(obj, SHT_STRTAB, name)
    {
        elf_section::put("", 1);       // NUL first string
    }

    // delete raw access
    void put(void const *, std::size_t) = delete;
    
    auto put(std::string const& s)
    {
        auto position = elf_section::position();
        elf_section::put(s.data(), s.size() + 1);
        return position;
    }

    auto put(const char *p)
    {
        auto position = elf_section::position();
        elf_section::put(p, std::strlen(p) + 1);
        return position;
    }
};

// create a symbol section
// NB: hold copy of symbols in host format, convert to target format
// on writing, or to host format on reading
struct es_symbol : elf_section
{
    // default initializer -- system symbol table
    es_symbol(elf_object& obj) : es_symbol(obj, ".symtab", ".strtab") {}

    // actual initializer
    es_symbol(elf_object& obj, std::string tab_name, std::string str_name)
        : sym_string (obj, str_name)
        , elf_section(obj, SHT_SYMTAB, tab_name, sizeof(Elf_Sym))
    {
        s_header.sh_link    = sym_string.index;
        s_header.sh_entsize = obj.cvt.tgt_size<Elf64_Sym>();

        // initial symbol is zero entry
        host_table.emplace_back();
    }
    
    // symbols use "add", not "put"
    void put(void const *, std::size_t) = delete;

    // add symbol to host symbol table
    Elf64_Word add(Elf64_Sym const& new_sym, std::string const& name = {})
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

    void generate_target(elf_object& obj)
    {
        // XXX if passthru just modify pointers...
        auto cnt = host_table.size();           // get symbol count...
        set_size(cnt * s_header.sh_entsize);    // ... and allocate memory
        
        // convert host -> target
        for (auto& s : host_table)
            elf_section::put(obj.cvt(s));
    }

    // make room for expected "entries"
    void reserve(Elf64_Word size)
    {
        host_table.reserve(size);
    }
private:
    std::vector<Elf64_Sym>  host_table;
    es_string    sym_string;        // support section for strings
};

}
#endif