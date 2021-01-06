#ifndef KBFD_KBFD_CONVERT_AOUT_H
#define KBFD_KBFD_CONVERT_AOUT_H


#include "elf_convert.h"
#include "aout_gnu.h"

namespace kbfd
{

struct aout_utils
{
    // a.out has header + fixed order sections:
    // declare them in the order written to output stream
    enum aout_sect { S_TEXT, S_DATA, S_BSS, S_T_REL, S_D_REL
                   , S_SYMTAB, S_STRTAB, S_UNDEF, N_AOUT_SECT };

    // name all `a.out` sections (static_assert enforced)
    static constexpr const char *aout_names[] = 
            { "S_TEXT", "S_DATA", "S_BSS", "S_T_REL", "S_D_REL"
            , "S_SYMTAB", "S_STRTAB", "S_UNDEF" };
    static_assert(std::extent_v<decltype(aout_names)> == N_AOUT_SECT);
   
    // pointers to `exec` members holding aout_sect sizes
    static constexpr unsigned exec::* aout_sizes[N_AOUT_SECT] = 
            { &exec::a_text, &exec::a_data, &exec::a_bss
            , &exec::a_trsize, &exec::a_drsize, &exec::a_syms };

    // declare mapping from "S_*" to a.out::"N_*" (most map to zero, ie N_UNDF)
    static constexpr uint8_t sect2reloc[N_AOUT_SECT] = { N_TEXT, N_DATA, N_BSS };

    // map elf section -> a.out section (enum `aout_sect`)
    static uint8_t from_elf(kbfd_section const& s)
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
    static uint8_t sym_type_from_elf(kbfd_object const& obj, Elf64_Sym const& sym)
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
                //return add_binding(N_UNDF, st_bind);
            case STT_SECTION:
                // symbol references a section.
                break;
            case STT_FILE:
                return N_FN;
            case STT_COMMON:
                return N_COMM;
        
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
                return add_binding(N_UNDF, st_bind);
            case SHN_COMMON:
                return N_COMM;
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
    // NB: swap not required: `a.out` types always in host order
    d.n_un.n_strx = sym.st_name + 4;      // offset for sizeof(string length field)
    d.n_type      = aout_utils::sym_type_from_elf(obj, sym);
    d.n_other     = sym.st_other;
    d.n_desc      = 0;                  // used by debugger (ie stabs)
    d.n_value     = sym.st_value;

    // special for commons: ELF `value` is alignment. A.OUT `value` is size
    if (d.n_type == N_COMM)
        d.n_value = sym.st_size;

    std::cout << "cvt_sym: from: " << std::make_pair(obj, sym);
    std::cout <<           " to: " << std::make_pair(obj, d);
    std::cout << std::endl;
}

// Relocation conversions are not as straightforward.
// `r_info` is composite of `sym` & `type`.
// `r_info` format is different for ELF32 & ELF64

template <>
inline void elf_convert::
    cvt_rel(relocation_info& d, Elf64_Rel const& s, bool src_is_host) const
{

    // extract sym number & type from `s`
    auto host_info = swap_src(s.r_info, true);  // know `src_is_host`
    auto sym_num   = ELF64_R_SYM(host_info);
    auto type      = ELF64_R_TYPE(host_info);
    const kas_reloc_info *info_p{};
    auto info_name = obj.relocs.get_info(type, &info_p);

    auto& sym      = obj.symtab_p->get(sym_num);

    // get reloc addend: see if symbol represents section
    auto aout_sect = aout_utils::sym_type_from_elf(obj, sym);
    switch (auto base_sect = aout_sect & ~N_EXT)
    {
        case N_TEXT:
        case N_DATA:
        case N_BSS:
        case N_ABS:
            d.r_symbolnum = base_sect;
            d.r_extern    = false;
            break;
        default:
            d.r_symbolnum = sym_num;
            d.r_extern    = true;
    }

    // examine `kas_reloc` for reloc method
    // NB: should error if no `info_p` or `method` not ADD
    if (info_p)
    {
        d.r_pcrel     = info_p->reloc.flags & kas_reloc::RFLAGS_PC_REL;
        d.r_length    = info_p->reloc.bits / 8;
    }
    
    // relocation address maps directly
    assign(d.r_address, s.r_offset, src_is_host);

    std::cout << "cvt_rel: from: " << std::make_pair(obj, s);
    std::cout <<           " to: " << std::make_pair(obj, d);
    std::cout << std::endl;
}

}

#endif
