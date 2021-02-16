#ifndef KBFD_KBFD_OBJECT_H
#define KBFD_KBFD_OBJECT_H

// kbfd_object
//
// The `kbfd_object` type holds a handle to an `elf` object.
// These can the output of as assembler, or the in-memory representation
// of objects read from disk for linking or other utilities.
//
// NB: If `ELF_FORMAT` describes a non-elf format, the object is 
//     converted to an `elf` compatible format on read, and then back
//     to base format on write.

#include "elf_common.h"
#include "kbfd_external.h"

#include <vector>

namespace kbfd
{

// forward declaration for `format` and member references
struct kbfd_target_format;
struct kbfd_reloc;
struct kbfd_target_reloc;
struct kbfd_convert;
struct swap_endian;

// forward declaration for `section` and derived sections
struct kbfd_section;
struct ks_symbol;
struct ks_string;

struct kbfd_object
{
    // declare appropriate type for "symbol number"
    using kbfd_sym_index_t = Elf64_Half;
    kbfd_object(kbfd_target_format const& fmt);

    std::size_t add_section_p(kbfd_section *p)
    {
        section_ptrs.emplace_back(p);
        return section_ptrs.size();
    }

    // optional method. reserve std::vector space
    void reserve_sections(Elf64_Word n_sections)
    {
        section_ptrs.reserve(n_sections);
    }

    const char *write(std::ostream&);
    
#if 0
    template <typename...Ts>
    decltype(auto) cvt(Ts&&...ts)
    {
        return fmt.cvt(std::forward<Ts>(ts)...);
    }
#else
  //  template <typename...Ts>
  //  cvt_src_rt cvt(Ts&&...ts) const;
#endif

    // lookup symbol name
    const char *sym_name(Elf64_Sym const& sym) const;
    const char *sym_name(Elf64_Word n_sym) const;

    // lookup relocation for target
    kbfd_target_reloc const *get_reloc(kbfd_reloc const&) const;
    kbfd_target_reloc const *get_reloc(uint16_t index) const;

    // instance data describing `object`
    Elf64_Ehdr e_hdr;           // kbfd (ie elf) header
    ks_symbol *symtab_p    {};  // symbol table for object
    ks_string *sh_string_p {};  // section name table
    
    // pointers to allocated sections
    std::vector<kbfd_section *> section_ptrs;
    
    // provide reverences to `fmt` members because incomplete type
    kbfd_target_format  const& fmt;
    kbfd_convert        const& cvt;
    swap_endian         const& swap;
};



}

#endif

