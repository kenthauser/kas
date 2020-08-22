#ifndef KAS_ELF_ELF_OBJECT_H
#define KAS_ELF_ELF_OBJECT_H

// elf_object
//
// The `elf_object` type holds a handle to an `elf` object.
// These can the output of as assembler, or the in-memory representation
// of objects read from disk for linking or other utilities.
//
// NB: If `ELF_FORMAT` describes a non-elf format, the object is 
//     converted to an `elf` compatible format on read, and then back
//     to base format on write.

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
    // declare appropriate type for "symbol number"
    using elf_sym_index_t = Elf64_Half;

    template <typename ELF_FORMAT>
    elf_object(ELF_FORMAT const& format)
        : swap(format.endian)
        , cvt(*this, swap, format)
        , e_hdr(format.header)
        , relocs(format.relocs)
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

    const char *sym_name(Elf64_Sym const& sym) const;
    const char *sym_name(Elf64_Word n_sym) const;

    Elf64_Ehdr e_hdr;       // elf header
    es_symbol *symtab_p    {};
    es_string *sh_string_p {};
    swap_endian swap;
    elf_convert cvt;
    elf_reloc_t const& relocs;
    std::vector<elf_section *> section_ptrs;
};



}

#endif

