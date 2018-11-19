#ifndef KAS_ELF_ELF_OBJECT_H
#define KAS_ELF_ELF_OBJECT_H

#include "elf_external.h"
#include "elf_common.h"
#include "elf_section.h"

namespace kas::elf
{

struct elf_file {

    // declare these as int to suppress narrowing messages.
    elf_file( int   ei_class
            , int   ei_data
            , int   e_machine
            , int   align = {}      // power of 2
            , int   ei_flags = {}
            , int   os_abi = {}
            , int   abi_version = {}
            )
            : cvt(ei_class, ei_data)
    {
        decltype(header.e_ident) ident = 
                { ELFMAG0, ELFMAG1, ELFMAG2, ELFMAG3
                  , static_cast<uint8_t>(ei_class)
                  , static_cast<uint8_t>(ei_data)
                  , EV_CURRENT
                  , static_cast<uint8_t>(os_abi)
                  , static_cast<uint8_t>(abi_version)
                  };
                  
        std::memmove(header.e_ident, ident, sizeof(ident));
        header.e_type    = ET_REL;      // Relocatable file
        header.e_machine = e_machine;
        header.e_version = EV_CURRENT;
        header.e_flags   = ei_flags;

        // calculated header size is 32/64 depending on ei_class
        header.e_ehsize    = cvt.header_size<Elf64_Ehdr>();
        header.e_phentsize = cvt.header_size<Elf64_Phdr>();
        header.e_shentsize = cvt.header_size<Elf64_Shdr>();

        // setup common data in sections
        elf_section::cvt_p = &cvt;

        // continue init  `kas_core` data structures (core_section && core_symbol)
        init_from_kas_core();
    }


    // INIT `elf` sections & symbols from `kas_core`
    void init_from_kas_core()
    {
        // 1. allocate `data_section` memory for maximum number of sections

        // allow for creation of 2 relocation sections for each data section
        // allow for "global" sections: eg .symtab, .strtab, .shstrtab
        auto SECTION_RESERVE_CNT  = 5;       // for globals...
             SECTION_RESERVE_CNT += core::core_section::num_objects() * 3; 

        data_sections.reserve(SECTION_RESERVE_CNT);

        // 2. create a ELF_SECTION for each core_section
        // NB: convention only. Can be removed.
        core::core_section::for_each([&](auto& s)
            { 
                data_sections.emplace_back(s); 
            });

        // 3. Calculate size of symbol table

        // count actual symbols
        Elf64_Half n_syms {};

        core::core_symbol::dump(std::cout);
       
        // XXX may be better to just use core_symbol::size()
        core::core_symbol::for_each_emitted([&n_syms](auto&...)
            {
                ++n_syms; 
            });

        // allocate initial dummy symbol & possible FILE symbol
        n_syms += 2;

        // symbols to act as proxys for data sections (and associated relocs)
        n_syms += SECTION_RESERVE_CNT;

        // 4. create the es_symbol section & store in `elf_section` static
        // NB: dynamically allocated so corresponding `elf_section` occurs
        //     after actual data sections, as appears is customary
        auto symtab_p = new es_symbol();
        elf_section::symtab_index = symtab_p->section.index();
        
        symtab_p->data.reserve(n_syms * symtab_p->s_header.sh_entsize);

        // 5. Add symbols: start with STT_FILE   
#ifdef ENFORCE_FILE_FIRST_LOCAL
        if (auto file_sym_p = core::core_symbol::file())
            symtab_p->add(*file_sym_p);
#else
        if (core::core_symbol::num_objects()) {
            auto& first = core::core_symbol::get(1);
            if (first.kind() == STT_FILE)
                symtab_p->add(first);
        }
#endif
        // 6. Add symbols: continue with STT_SECTION proxies
        for (auto& d : data_sections)
            d.sym_num = symtab_p->add(d);

        // 7. Add symbols: continue with STB_LOCAL symbols from `core_symbol`
        core::core_symbol::for_each_emitted([symtab_p](auto& sym)
            {
                if (sym.binding() == STB_LOCAL)
                    if (!sym.sym_num())
                        if (symtab_p->should_emit_local(sym))
                            symtab_p->add(sym);
            });

        // 8. Add symbols: finish with remaining symbols from `core_symbol`
        core::core_symbol::for_each_emitted([symtab_p](auto& sym)
            {
                if (!sym.sym_num())
                    if (symtab_p->should_emit_non_local(sym))
                        symtab_p->add(sym);
            });

        // 9. done
        core::core_symbol::dump(std::cout);
    }



    void write(std::ostream& os)
    {
        // create section_name section, if needed
        if (!sh_string)
            sh_string = new es_string(".shstrtab");

        // initialize section names (as required)
        // side-effect: generate secondary names (such as `.rel.text`)
        elf_section::for_each([&](auto& s)
            {
                if (!s.s_header.sh_name)
                    s.s_header.sh_name = sh_string->put(s.name);
            });

        // initialize ELF header section indexes (+1 for initial zero section)
        header.e_shnum      = elf_section::num_objects() + 1;
        header.e_shstrndx   = sh_string->section.index();

        // clear ph_entsize if no program headers (seems customary, but not required...)
        if (header.e_phnum == 0)
            header.e_phentsize = 0;

        // calculate physical offsets
        auto offset = header.e_ehsize;
        elf_section::for_each([&](auto& s)
            {
                // calculate padding needed to align section data
                if (s.s_header.sh_type != SHT_NOBITS) {
                    s.padding = cvt.padding(s.s_header.sh_addralign, offset);
                    offset += s.padding;
                }

                // record offsets in section
                s.s_header.sh_offset = offset;
                s.s_header.sh_size   = s.size();
                
                // NO_BITS sections don't occupy space
                if (s.s_header.sh_type != SHT_NOBITS)
                    offset += s.s_header.sh_size;
            });
    
        // align section headers after data
        auto h_alignment = cvt.header_align<Elf64_Shdr>();
        auto h_padding   = cvt.padding(h_alignment, offset);
        header.e_shoff   = offset + h_padding;

        // now write data: first elf_header
        os.write((const char *)cvt.cvt(header), header.e_ehsize);

        // next write data: section data
        elf_section::for_each([&os, &cvt=cvt](auto& s)
            {
                if (s.s_header.sh_type == SHT_NOBITS)
                    return;
                if (s.padding)
                    os.write(cvt.zero, s.padding);
                os.write(s.data.data(), s.s_header.sh_size);
            });


        // next write data: section headers
        // align if needed
        if (h_padding)
            os.write(cvt.zero, h_padding);

        // first is zero entry
        os.write(cvt.zero, header.e_shentsize);

        // followed by actual section headers
        elf_section::for_each([&os, &cvt=cvt, &header=header](auto& s)
            {
                // sigh...why is `void *` not opaque
                auto header_p = static_cast<char const *>(cvt.cvt(s.s_header));
                os.write(header_p, header.e_shentsize);
            });
        os.flush();     // push to device
    }

    Elf64_Ehdr  header {};
    const elf_convert cvt;
    std::vector<es_data> data_sections;
    es_string *sh_string {};
};


}

#endif
