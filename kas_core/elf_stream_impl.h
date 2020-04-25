#ifndef KAS_ELF_ELF_STREAM_IMPL_H
#define KAS_ELF_ELF_STREAM_IMPL_H

#include "elf_stream.h"

namespace kas::core
{

// INIT `elf` sections & symbols from `kas_core`
template <typename ELF_OBJECT>
elf_stream::elf_stream(ELF_OBJECT& object)
        : object(object)
        , swap(object.swap)
{
    // 1. allocate `data_section` memory for maximum number of sections
    // NB: as `elf_object` uses std::vector to hold pointers to sections,
    // NB: relatively cheap to resize vector. This step may be omitted.

    // allow for creation of 2 relocation sections for each data section
    // allow for "global" sections: eg .symtab, .strtab, .shstrtab
    auto SECTION_RESERVE_CNT  = 5;       // for global sections...
         SECTION_RESERVE_CNT += core::core_section::num_objects() * 3; 

    object.reserve_sections(SECTION_RESERVE_CNT);

    // 2. create a ELF_SECTION for each core_section
    // NB: This causes data sections of object to be first
    // NB: sections in ELF object in same order as assembler.
    // NB: This is a convention & this step may be safely omitted.
    core::core_section::for_each([&](auto& s)
        { 
            core2es_data(s);        // creates elf_sections on reference
        });

    // 3. Calculate size of symbol table
    // count actual symbols
    elf::Elf64_Half n_syms {};

    core::core_symbol_t::dump(std::cout);
   
    // XXX may be better to just use core_symbol::size()
    core::core_symbol_t::for_each_emitted([&n_syms](auto&...)
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
    object.symtab_p = new elf::es_symbol(object);
    object.symtab_p->reserve(n_syms);          // reserve entries

    // 5. Add symbols: start with STT_FILE   
#ifdef ENFORCE_FILE_FIRST_LOCAL
    // retrieve `STT_FILE` from special location
    if (auto file_sym_p = core::core_symbol::file())
        add_sym(*file_sym_p);
#else
    // if first symbol is `STT_FILE`, emit
    if (core::core_symbol_t::num_objects() != 0)
    {
        auto& first = core::core_symbol_t::get(1);
        if (first.kind() == STT_FILE)
            add_sym(first);
    }
#endif
    // 6. Add symbols: continue with STT_SECTION proxies
    core::core_section::for_each([&](auto& s)
        {
            add_sym(s);     // `elf_stream` local method
        });

    // 7. Add symbols: continue with STB_LOCAL symbols from `core_symbol`
    core::core_symbol_t::for_each_emitted([this](auto& sym)
        {
            if (sym.binding() == STB_LOCAL)
                if (!sym.sym_num())
                    if (should_emit_local(sym))
                        add_sym(sym);
        });

    // 8. Add symbols: finish with remaining symbols from `core_symbol`
    core::core_symbol_t::for_each_emitted([&](auto& sym)
        {
            if (!sym.sym_num())
                if (should_emit_non_local(sym))
                    add_sym(sym);
        });

    // 9. done
    core::core_symbol_t::dump(std::cout);
}
    
// construct new ELF data section from `core::core_section` section
auto elf_stream::core2es_data(core::core_section const& s) const -> elf::es_data& 
{
    // retrieve `es_data *` using callback (if previously created)
    auto p = static_cast<elf::es_data *>(s.elf_callback());
    
    if (!p)
    {
        // construct new `es_data` section in `elf_object` from `core_section`
        p = new elf::es_data(object, s.sh_type, s.sh_name, s.kas_align);
        p->s_header.sh_flags = s.sh_flags;      // R/W, EXEC, ALLOC, etc

        // XXX group/linkage?
        // XXX st_link, st_info

        p->set_size(s.size());      // allocate memory to hold section data
        s.set_elf_callback(p);      // register callback
    }
    return *p;
}

// binutils defn uses `bfd_vma`. override with the ELF standard defn
#undef  ELF64_R_INFO
#define ELF64_R_INFO(s,t)   (((s) << 32) + (t))

// actually emit a reloc
// Generate an ELF64 relocation & then convert to target format
void elf_stream::put_elf_reloc(core::e_chan_num num
                , uint32_t reloc
                , uint32_t sym_num
                , uint8_t  offset
                , int64_t& data
                ) const
{
    static constexpr auto use_rel_a = false;
    
    if (num != core::EMIT_DATA)
        return;

#if 0
    if (!use_rel_a && !format.ok_for_relocation(data))
        generate error
    // should probably be done by `core_emit`...
#endif
    
    // NB: If host_format changes from ELF64, need to revisit code.
    // NB: Don't parameterize types, instead generate compilation
    // NB: errors on host type change

    // can't shift a uint32 by 32. convert to uint64 before shift
    elf::Elf64_Xword r_sym  = sym_num;
    elf::Elf64_Xword r_info = ELF64_R_INFO(r_sym, reloc);

    // XXX offset can use some analysis for `endian` issues...

    if (!use_rel_a)
    {
        // create ELF64_Rel `reloc`
        elf::Elf64_Rel reloc{ position() + offset, r_info };
        std::cout << "put_elf_reloc: r_info = " << std::hex << r_info;
        std::cout << ", sym = " << sym_num;
        std::cout << ", offset = " << std::hex << reloc.r_offset;
        std::cout << ", info = " << reloc.r_info << std::endl;
        es_data_p->put_reloc(reloc);
    }
    else
    {
        // create ELF64_Rela `reloc`
        elf::Elf64_Rela reloc{ position() + offset, r_info, data };
        es_data_p->put_reloc_a(reloc);
        data = 0;
    }
}

}

#endif
