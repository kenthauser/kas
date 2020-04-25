#ifndef KAS_ELF_ELF_OBJECT_H
#define KAS_ELF_ELF_OBJECT_H

#include "elf_common.h"
#include "elf_external.h"

namespace kas::elf
{

// forward declarations
struct elf_section;
struct es_symbol;
struct es_string;

struct elf_object
{

    template <typename ELF_FORMAT>
    elf_object(ELF_FORMAT const& format)
        : swap(format.endian)
        , cvt(swap, format)
        , e_hdr(format.header)
    {}

    std::size_t add_section_p(elf_section *p)
    {
        section_ptrs.emplace_back(p);
        return section_ptrs.size();
    }

    // optional method. vector resizes automatically
    void reserve_sections(Elf64_Word n_sections)
    {
        section_ptrs.reserve(n_sections);
    }

    void write(std::ostream& os);

    Elf64_Ehdr e_hdr;       // elf header
    es_symbol *symtab_p;
    es_string *sh_string_p; // XXX for write
    swap_endian swap;
    elf_convert cvt;
    
private:
    std::vector<elf_section *> section_ptrs;
};



}

#endif

