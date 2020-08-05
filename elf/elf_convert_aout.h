#ifndef KAS_ELF_ELF_CONVERT_AOUT_H
#define KAS_ELF_ELF_CONVERT_AOUT_H

// Convert ELF Class (ELF32/ELF64) and data ENDIAN between host and target formats
//
// When headers are in memory, they are stored as ELF64 with host-endian.
// Data in sections is always stored in target format (class/endian)
// `elf_convert` supports both host->target & target->host translations
// of headers & byte-swap for endian conversion of data emited to data sections.

// Conversions between various ELF formats is generally just a member-wise
// assignment between source and destination, possibly with endian conversion
//
// The exception is the relocation formats beause the `r_info` member formats
// are slightly different.

#include "elf_convert.h"
#include "aout_gnu.h"

namespace kas::elf
{

// make dst member `cast`ed & `swap`ed from src member.
#define ASSIGN(member)  assign(d.member, s.member, src_is_host)
#if 0
// Elf Ehdr
template <typename DST, typename SRC>
void elf_convert::cvt_ehdr(DST& d, SRC const& s, bool src_is_host) const
{
    std::memcpy(d.e_ident, s.e_ident, EI_NIDENT);
    ASSIGN(e_type);
    ASSIGN(e_machine);
    ASSIGN(e_version);
    ASSIGN(e_entry);
    ASSIGN(e_phoff);
    ASSIGN(e_shoff);
    ASSIGN(e_flags);
    ASSIGN(e_ehsize);
    ASSIGN(e_phentsize);
    ASSIGN(e_phnum);
    ASSIGN(e_shentsize);
    ASSIGN(e_shnum);
    ASSIGN(e_shstrndx);
}

// Elf Shdr
template <typename DST, typename SRC>
void elf_convert::cvt_shdr(DST& d, SRC const& s, bool src_is_host) const
{
    ASSIGN(sh_name);
    ASSIGN(sh_type);
    ASSIGN(sh_flags);
    ASSIGN(sh_addr);
    ASSIGN(sh_offset);
    ASSIGN(sh_size);
    ASSIGN(sh_link);
    ASSIGN(sh_info);
    ASSIGN(sh_addralign);
    ASSIGN(sh_entsize);
}
#endif
// generate a.out symbols
#if 0
template <typename DST, typename SRC>
void elf_convert::cvt_sym(DST& d, SRC const& s, bool src_is_host) const
#else
template <>
void elf_convert::cvt_sym(nlist& d, Elf64_Sym const& s, bool src_is_host) const
#endif
{
    //static_assert (std::is_same_v<DST, nlist>);
#if 0
    ASSIGN(st_name);
    ASSIGN(st_value);
    ASSIGN(st_size);
    ASSIGN(st_info);
    ASSIGN(st_other);
    ASSIGN(st_shndx);
#endif
}
#if 0
// Elf Phdr
template <typename DST, typename SRC>
void elf_convert::cvt_phdr(DST& d, SRC const& s, bool src_is_host) const
{
    ASSIGN(p_type);
    ASSIGN(p_offset);
    ASSIGN(p_vaddr);
    ASSIGN(p_paddr);
    ASSIGN(p_filesz);
    ASSIGN(p_memsz);
    ASSIGN(p_flags);
    ASSIGN(p_align);
}
#endif
// Relocation conversions are not as straightforward.
// `r_info` is composite of `sym` & `type`.
// `r_info` format is different for ELF32 & ELF64

#if 0
// Relocation conversion: SRC & DST are same ei_class (ie host format)
// endian swap can still be required
template <>
inline void elf_convert::
    cvt_rel(Elf64_Rel& d, Elf64_Rel const& s, bool src_is_host) const
{
    ASSIGN(r_offset);
    ASSIGN(r_info);
}
template <>
inline void elf_convert::
    cvt_rel(Elf64_Rela& d, Elf64_Rela const& s, bool src_is_host) const
{
    ASSIGN(r_offset);
    ASSIGN(r_info);
    ASSIGN(r_addend);
}
#endif
// Relocation conversion: DST = ELF32, SRC = ELF64
// NB: make a `template` for single definition purposes

template <>
inline void elf_convert::
    cvt_rel(relocation_info& d, Elf64_Rel const& s, bool src_is_host) const
{
#if 0
    auto host_info = swap_src(s.r_info, true);  // know `src_is_host`
    auto sym  = ELF64_R_SYM(host_info);
    auto type = ELF64_R_TYPE(host_info);

    //  use DST size for result
    decltype(d.r_info) info = ELF32_R_INFO(sym, type);
    d.r_info = swap_dst(info, true);
    
    ASSIGN(r_offset);
#endif
}
template <>
inline void elf_convert::
    cvt_rela(Elf32_Rela& d, Elf64_Rela const& s, bool src_is_host) const
{
#if 0
    auto host_info = swap_src(s.r_info, true);  // know `src_is_host`
    auto sym  = ELF64_R_SYM(host_info);
    auto type = ELF64_R_TYPE(host_info);

    //  use DST size for result
    decltype(d.r_info) info = ELF32_R_INFO(sym, type);
    d.r_info = swap_dst(info, true);

    ASSIGN(r_offset);
    ASSIGN(r_addend);
#endif
}

#if 0
// Relocation conversion: DST = ELF64, SRC = ELF32
// NB: make a `template` for single definition purposes
template <typename DST, typename>
void const *elf_convert::hdr_cvt(DST& d, Elf32_Rel const& s, Elf_Rel) const
{
    auto host_info = swap_src(s.r_info);        // get info to host format
    auto sym  = ELF32_R_SYM(host_info);
    auto type = ELF32_R_TYPE(host_info);

    // use DST size for result
    decltype(d.r_info) info = ELF64_R_INFO(sym, type);
    d.r_info = swap_dst(info);

    ASSIGN(r_offset);
    return &d;
}
template <typename DST, typename>
void const *elf_convert::hdr_cvt(DST& d, Elf32_Rela const& s, Elf_Rela) const
{
    auto host_info = swap_src(s.r_info);        // get info to host format
    auto sym  = ELF32_R_SYM(host_info);
    auto type = ELF32_R_TYPE(host_info);

    // use DST size for result
    decltype(d.r_info) info = ELF64_R_INFO(sym, type);
    d.r_info = swap_dst(info);

    ASSIGN(r_offset);
    ASSIGN(r_addend);
    return &d;
}
#endif
// remove macro
#undef ASSIGN

}

#endif
