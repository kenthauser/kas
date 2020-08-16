#ifndef KAS_ELF_ELF_FORMAT_ELF_WRITE_H
#define KAS_ELF_ELF_FORMAT_ELF_WRITE_H

#include "elf_format_elf.h"
#include "elf_section.h"
#include "elf_convert.h"
#include "elf_section_sym.h"

namespace kas::elf
{

template <std::endian ENDIAN, typename HEADERS, typename...Ts>
void elf_format_elf<ENDIAN, HEADERS, Ts...>::
    write(elf_object& obj, std::ostream& os) const
{
    // write a std::pair<void const *, std::size> pair
    auto do_write = [](auto& os, std::pair<void const *, std::size_t>d)
        {
            auto p = static_cast<const char *>(d.first);
            os.write(p, d.second);
        };
   
    // create convenient references to `obj` members
    auto& e_hdr        = obj.e_hdr;
    auto& symtab_p     = obj.symtab_p;
    auto& cvt          = obj.cvt;
    auto& sh_string_p  = obj.sh_string_p;
    auto& section_ptrs = obj.section_ptrs;

    // create section_name section, if needed
    if (!sh_string_p)
        sh_string_p = new es_string(obj, ".shstrtab");

    // initialize section names (as required)
    for (auto& p : section_ptrs)
    {
        if (!p->s_header.sh_name)
            p->s_header.sh_name = sh_string_p->put(p->name);
    };

    
    // initialize ELF header section indexes (+1 for initial zero section)
    e_hdr.e_shnum      = section_ptrs.size() + 1;
    e_hdr.e_shstrndx   = sh_string_p->index;

    // clear ph_entsize if no program headers (seems customary, but not required...)
    if (e_hdr.e_phnum == 0)
        e_hdr.e_phentsize = 0;

    // perform required conversions & calculate physical offsets
    auto offset = e_hdr.e_ehsize;

    for (auto& p : section_ptrs)
    {
        std::cout << "sections: name = " << p->name << ", offset = " << offset << std::endl;
        // perform host -> target conversions for symbol tables & relocations
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

        // calculate padding needed to align section data
        if (p->s_header.sh_type != SHT_NOBITS)
        {
            p->padding = cvt.padding(p->s_header.sh_addralign, offset);
            offset += p->padding;
        }
        
        // record offsets in section
        p->s_header.sh_offset = offset;
        p->s_header.sh_size   = p->position();  // XXX misuse of method?
        
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
#if 1
    // now write data: first elf_header
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
#if 1
    // first is zero entry
    std::cout << "e_shentsize " << +e_hdr.e_shentsize << std::endl;
    os.write(cvt.zero, e_hdr.e_shentsize);
#endif
#if 1
    // followed by actual section headers
    for (auto& p : section_ptrs)
    {
        do_write(os, cvt(p->s_header));
    };
    
#endif
    std::cout << "sections: " << std::endl;
    for (auto& p : section_ptrs)
    {
        //std::cout << (p - &section_ptrs[0]) << ":"
        std::cout << " name = " << p->name;
        std::cout << " sh_name = " << (sh_string_p->data.data() + p->s_header.sh_name);
        std::cout << " sh_offset = " << p->s_header.sh_offset;
        std::cout << " sh_size = "  << p->s_header.sh_size;
        std::cout << std::endl;
    }
    os.flush();     // push to device
}

}

#endif
