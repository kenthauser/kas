#ifndef KBFD_KBFD_FORMAT_AOUT_WRITE_H
#define KBFD_KBFD_FORMAT_AOUT_WRITE_H

#include "kbfd_format_aout.h"
#include "kbfd_section.h"
#include "elf_convert.h"
#include "kbfd_section_sym.h"

namespace kbfd
{

template <std::endian ENDIAN, typename HEADERS, typename...Ts>
void kbfd_format_aout<ENDIAN, HEADERS, Ts...>::
    write(kbfd_object& obj, std::ostream& os) const
{
    using utils = aout_utils;
    
    // map elf sections -> a.out sections. ignore empty (eg consolidated) sections
    std::array<kbfd_section *, utils::N_AOUT_SECT> aout_sections{}; 
    for (auto p : obj.section_ptrs)
    {
        auto save = [&aout_sections](kbfd_section *p, unsigned n)
            {
                auto& sp = aout_sections[n];
                if (sp)
                    throw std::runtime_error("a.out: duplicate section: " + p->name);
                sp = p;
            };
        auto sect = utils::from_elf(*p);
        std::cout << "a.out: mapping " << p->name;
        std::cout << " to " << utils::aout_names[sect] << std::endl;
    }
    
    // generate "symbols" && "relocs" sections
    for (auto p : obj.section_ptrs)
    {
        switch (p->s_header.sh_type)
        {
            case SHT_SYMTAB:
                ks_symbol::gen_target_data(obj, p);
                break;
            case SHT_REL:
                detail::ks_reloc<Elf64_Rel>::gen_target_data(obj, p);
                break;
            case SHT_RELA:
                detail::ks_reloc<Elf64_Rela>::gen_target_data(obj, p);
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
    
    // initialize the `a.out` header
    exec aout_header{};
    N_SET_INFO(&aout_header, OMAGIC, M_UNKNOWN, 0);

    // calculate physical offsets
    auto offset = sizeof(exec);

    auto n = 0;
    for (auto& p : aout_sections)
    {
        ++n;
        if (!p) continue;       // ignore empty sections
        
        std::cout << "a.out: section: n = " << (n-1) << std::endl;
        std::cout << "sections: name = " << p->name << ", offset = " << offset;
        std::cout << std::endl;

        // XXX this should be in common method...
        // calculate padding needed to align section data
        if (p->s_header.sh_type != SHT_NOBITS)
        {
            p->padding = cvt.padding(p->s_header.sh_addralign, offset);
            offset += p->padding;
        }
        
        // record offsets in section
        p->s_header.sh_offset = offset;
        p->s_header.sh_size   = p->position();      // size is next write offset
       
        // store section size in `a.out` header
        if (auto mp = utils::aout_sizes[n-1])
            aout_header.*mp = p->size();

        // NO_BITS sections don't occupy space
        if (p->s_header.sh_type != SHT_NOBITS)
            offset += p->s_header.sh_size;
    };
    
    
    // write a (void const *, std::size) pair
    auto do_write = [&os](void const *p, std::size_t s)
        {
            os.write(static_cast<const char *>(p), s);
        };
    
    // now write data: first aout_header
    do_write(&aout_header, sizeof(aout_header));

    // write section data in order (except BSS)
    aout_sections[utils::S_BSS] = {};
    for (auto p : aout_sections)
        if (p)
            do_write(p->begin(), p->size());

    // finally write string table prefixed by "size" (32-bits)
    auto& strtab = obj.symtab_p->strtab();
    uint32_t str_size = strtab.size() + 4;    // add sizeof(str_size)
    do_write(&str_size, sizeof(str_size));    // size in host order
    do_write(strtab.begin(), strtab.size());  // write string table

    os.flush();     // push to device
}

}

#endif
