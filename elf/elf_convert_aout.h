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

struct aout_utils
{
    // a.out has header + fixed order sections:
    enum aout_sect { S_TEXT, S_DATA, S_BSS, S_T_REL, S_D_REL
                   , S_SYMTAB, S_STRTAB, S_UNDEF, N_AOUT_SECT };

    static constexpr const char *aout_names[N_AOUT_SECT] = 
            { "S_TEXT", "S_DATA", "S_BSS", "S_T_REL", "S_D_REL"
            , "S_SYMTAB", "S_STRTAB", "S_UNDEF" };

    // declare mapping from "S_*" to a.out::"N_*"
    static constexpr uint8_t sect2reloc[N_AOUT_SECT] = { N_TEXT, N_DATA, N_BSS };

    // map elf section -> a.out section (enum `aout_sect`)
    static uint8_t from_elf(elf_section const& s)
    {
        // deduce a.out section from `s_header` type
        auto& obj     = s.object;           // object container for section
        auto sh_type  = s.s_header.sh_type;
        auto sh_flags = s.s_header.sh_flags;

        switch (sh_type)
        {
            case SHT_PROGBITS:
                // should only be 1 each text & data section
                // multiple sections are errored out by `...aout_write.h`
                if (sh_flags == SHF_ALLOC+SHF_EXECINSTR)
                    return S_TEXT;
                if (sh_flags == SHF_ALLOC+SHF_WRITE)
                    return S_DATA;
                return S_UNDEF;
            case SHT_SYMTAB:
                return S_SYMTAB;
            case SHT_STRTAB:
                return S_STRTAB;    // should make sure it's for symtab
            case SHT_REL:
                break;
            case SHT_NOBITS:
                if (sh_flags == (SHF_ALLOC+SHF_WRITE))
                    return S_BSS;
            // FALLSTHRU
            default:
                return S_UNDEF;
        }

        // here examining `SHT_REL` section.
        // need to examine related section to determine section
        auto p = obj.section_ptrs[s.s_header.sh_info-1];    // XXX
        auto p_type  = p->s_header.sh_type;
        auto p_flags = p->s_header.sh_flags;

        if (p_type == SHT_PROGBITS)
        {
            if (p_flags == SHF_ALLOC+SHF_EXECINSTR)
                return S_T_REL;
            if (p_flags == SHF_ALLOC+SHF_WRITE)
                return S_D_REL;
        }
        
        return S_UNDEF; 
    }

    // map elf symbol -> a.out nlist `n_type`
    // NB: incomplete: no support for weak symbols, stabs, or fancy relocs
    static uint8_t sym_type_from_elf(elf_object const& obj, Elf64_Sym const& sym)
    {
        auto add_binding = [](uint8_t type, uint8_t bind)
            {
                // ignore `weak`
                if (bind == STB_GLOBAL)
                    type |= N_EXT;      // ie 1
                return type;
            };

        auto st_type = ELF64_ST_TYPE(sym.st_info);
        auto st_bind = ELF64_ST_BIND(sym.st_info);


        // see if `symbol` has well-known type
        switch (st_type)
        {
            case STT_NOTYPE:
            case STT_OBJECT:
            case STT_FUNC:
                return add_binding(N_UNDF, st_bind);
            case STT_SECTION:
                // symbol references a section.
                break;
            case STT_FILE:
                return N_FN;
            case STT_COMMON:
                return add_binding(N_UNDF, STB_GLOBAL);
        
            // unsupported
            case STT_TLS:
            case STT_RELC:
            case STT_SRELC:
            case STT_LOOS:
            //case STT_GNU_IFUNC: duplicate case of `STT_LOOS`
            case STT_HIOS:
            case STT_LOPROC:
            case STT_HIPROC:
            default:
                // XXX throw?
                return 0;
        }
        
        // get `sym_type` by looking up symbols's section
        // first check for "reserved" section numbers
        switch (sym.st_shndx)
        {
            case SHN_UNDEF:
            case SHN_COMMON:
                return add_binding(N_UNDF, STB_GLOBAL);
            case SHN_ABS:
                return add_binding(N_ABS, st_bind);
            default:
                break;
        }

        // see if proper section, or reserved (ie unsupported by a.out)
        if (sym.st_shndx >= SHN_LORESERVE)
            return 0;   // XXX throw?

        // lookup symbol and retrun section based on corresponding section
        auto p = obj.section_ptrs[sym.st_shndx-1];
        if (!p) return 0;   // XXX throw
        
        auto aout_sect = from_elf(*p);      // get aout_sect enum
        return add_binding(sect2reloc[aout_sect], st_bind);
    }
};


// generate a.out symbols
template <>
void elf_convert::cvt_sym(nlist& d, Elf64_Sym const& sym, bool src_is_host) const
{
    // NB: swap not required: `a.out` headers always in host order
    d.n_un.n_strx = sym.st_name + 4;      // offset for sizeof(string length field)
    d.n_type      = aout_utils::sym_type_from_elf(obj, sym);
    d.n_other     = sym.st_other;
    d.n_desc      = 0;                  // used by debugger (ie stabs)
    d.n_value     = sym.st_value;

    std::cout << "cvt_sym: from: " ;//<< sym << " to: " << std::hex;
    std::cout << " n_type = " << +d.n_type;
    std::cout << " n_value = " << d.n_value;
    std::cout << std::endl;
}

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

    // extract sym number & type from `s`
    auto host_info = swap_src(s.r_info, true);  // know `src_is_host`
    auto sym  = ELF64_R_SYM(host_info);
    auto type = ELF64_R_TYPE(host_info);

    std::cout << "aout::cvt_rel: " << std::hex;
    std::cout << " sym = " << sym;
    std::cout << " type = " << type;
    std::cout << " addr = " << s.r_offset;
    std::cout << std::endl;
#if 0
    //  use DST size for result
    decltype(d.r_info) info = ELF32_R_INFO(sym, type);
    d.r_info = swap_dst(info, true);
#endif
    unsigned r_symbolnum;
        

    assign(r_symbolnum, sym, src_is_host);
    assign(d.r_address, s.r_offset, src_is_host);
    d.r_symbolnum = r_symbolnum;
}

template <>
inline void elf_convert::
    cvt_rela(relocation_info& d, Elf64_Rela const& s, bool src_is_host) const
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

}

#endif
