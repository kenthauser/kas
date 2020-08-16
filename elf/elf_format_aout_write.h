#ifndef KAS_ELF_ELF_FORMAT_AOUT_WRITE_H
#define KAS_ELF_ELF_FORMAT_AOUT_WRITE_H

#include "elf_format_aout.h"
#include "elf_section.h"
#include "elf_convert.h"
#include "elf_section_sym.h"

namespace kas::elf
{

template <std::endian ENDIAN, typename HEADERS, typename...Ts>
void elf_format_aout<ENDIAN, HEADERS, Ts...>::
    write(elf_object& obj, std::ostream& os) const
{
    using a = aout_utils;
    
    // map elf sections -> a.out sections. ignore empty (eg consolidated) sections
    std::array<elf_section *, a::N_AOUT_SECT> aout_sections{}; 
    for (auto p : obj.section_ptrs)
    {
        auto save = [&aout_sections](elf_section *p, unsigned n)
            {
                auto& sp = aout_sections[n];
                if (sp)
                    throw std::runtime_error("a.out: duplicate section:" + p->name);
                sp = p;
            };
#if 0
        // XXX alternately use ST_TYPE to classify sections...
        std::cout << "a.out: map: " << p->name << std::endl;
        if (p->empty())
            continue;                       // nothing to see here
        else if (p->name == ".text")
            save(p, S_TEXT);
        else if (p->name == ".data")
            save(p, S_DATA);
        else if (p->name == ".bss")
            save(p, S_BSS);
        else if (p->name == ".rel.text")
            save(p, S_T_REL);
        else if (p->name == ".rel.data")
            save(p, S_D_REL);
        else if (p->name == ".rel.bss")
            continue;                       // NO_BITS is NO_BITS...
        else if (p->name == ".rela.bss")
            continue;                       // NO_BITS is NO_BITS...
        else if (p->name == ".symtab")
            save(p, S_SYM);
        else if (p->name == ".strtab")
            save(p, S_STRTAB);
        else if (p->name == ".shstrtab")    // ELF section table name table
            continue;                       // (if converting from ELF)
        else
            throw std::runtime_error{"a.out: section not supported: " + p->name};
#else
        auto sect = a::from_elf(*p);
        std::cout << "a.out: mapping " << p->name << " to " << a::aout_names[sect] << std::endl;
#endif
    }
    
    // generate "symbols" && "relocs" sections
    for (auto p : obj.section_ptrs)
    {
        switch (p->s_header.sh_type)
        {
            case SHT_SYMTAB:
                es_symbol::gen_target_data(obj, p);
                break;
            case SHT_REL:
                detail::es_reloc<Elf64_Rel>::gen_target_data(obj, p);
                break;
            case SHT_RELA:
                detail::es_reloc<Elf64_Rela>::gen_target_data(obj, p);
                break;
            default:
                break;
        }
    }
    
    // create convenient references to `obj` members
    auto& e_hdr        = obj.e_hdr;
    auto& symtab_p     = obj.symtab_p;
    auto& cvt          = obj.cvt;
    auto& sh_string_p  = obj.sh_string_p;
    auto& section_ptrs = obj.section_ptrs;
    
    // calculate physical offsets
    auto offset = e_hdr.e_ehsize;

    auto n = 0;
    for (auto& p : aout_sections)
    {
        ++n;
        if (!p) continue;       // ignore empty sections
        
        std::cout << "a.out: section: n = " << (n-1) << std::endl;
        std::cout << "sections: name = " << p->name << ", offset = " << offset << std::endl;
        // calculate padding needed to align section data
        if (p->s_header.sh_type != SHT_NOBITS)
        {
            p->padding = cvt.padding(p->s_header.sh_addralign, offset);
            offset += p->padding;
        }
        
        // record offsets in section
        p->s_header.sh_offset = offset;
        p->s_header.sh_size   = p->position();      // size is next write offset
        
        // NO_BITS sections don't occupy space
        if (p->s_header.sh_type != SHT_NOBITS)
            offset += p->s_header.sh_size;
    };
#if 0
    // align section headers after data
    auto h_alignment = cvt.header_align<Elf64_Shdr>();
    auto h_padding   = cvt.padding(h_alignment, offset);
    header.e_shoff   = offset + h_padding;
#else
    e_hdr.e_shoff = offset;
#endif
    
    // now write data: first aout_header
    
    // write a std::pair<void const *, std::size> pair
    auto do_write = [](auto& os, std::pair<void const *, std::size_t> d)
        {
            auto p = static_cast<const char *>(d.first);
            os.write(p, d.second);
        };

    // initialize the `a.out` header
    exec aout_header{};
    N_SET_INFO(&aout_header, OMAGIC, M_UNKNOWN, 0);
#if 0 
    if (auto p = aout_sections[S_TEXT])
        aout_header.a_text   = p->size();
    if (auto p = aout_sections[S_DATA])
        aout_header.a_data = p->size();
    if (auto p = aout_sections[S_BSS])
        aout_header.a_bss   = p->size();
    if (auto p = aout_sections[S_T_REL])
        aout_header.a_trsize   = p->size();
    if (auto p = aout_sections[S_D_REL])
        aout_header.a_drsize   = p->size();
    if (auto p = aout_sections[S_SYM])
        aout_header.a_syms   = p->size();
#endif
    do_write(os, {&aout_header, sizeof(aout_header)});

    // write section data in order (except BSS)
    aout_sections[a::S_BSS] = {};
    for (auto p : aout_sections)
    {
        if (p && p->size())
            do_write(os, {p, p->size()});
    }
#if 0
    //os.write((const char *)cvt.cvt(header), header.e_ehsize);
    do_write(os, cvt(e_hdr));
    // next write data: section data
    for (auto& p : section_ptrs)
    {
        if (p->s_header.sh_type == SHT_NOBITS)
            continue;
        if (p->padding)
            os.write(cvt.zero, p->padding);
        if (p->position())
            os.write(p->begin(), p->position());
    };
#endif
#if 0

    // next write data: section headers
    // align if needed
    if (h_padding)
        os.write(cvt.zero, h_padding);
#endif
#if 0
    // first is zero entry
    std::cout << "e_shentsize " << +e_hdr.e_shentsize << std::endl;
    os.write(cvt.zero, e_hdr.e_shentsize);
#endif
#if 0
    // followed by actual section headers
    for (auto& p : section_ptrs)
    {
        do_write(os, cvt(p->s_header));
    };
    
#endif
    os.flush();     // push to device
}

}

#endif
