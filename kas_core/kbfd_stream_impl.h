#ifndef KAS_CORE_KBFD_STREAM_IMPL_H
#define KAS_CORE_KBFD_STREAM_IMPL_H

#include "kbfd_stream.h"

namespace kas::core
{

// INIT `elf` sections & symbols from `kas_core`
template <typename ELF_OBJECT>
kbfd_stream::kbfd_stream(ELF_OBJECT& object)
        : object(object)
        , swap(object.swap)
{
    // 1. allocate `data_section` memory for maximum number of sections
    // NB: as `kbfd_object` uses std::vector to hold pointers to sections,
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
            core2ks_data(s);        // creates kbfd_sections on reference
        });

    // 3. Calculate size of symbol table
    // count actual symbols
    kbfd::kbfd_object::kbfd_sym_index_t n_syms {};

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

    // 4. create the ks_symbol section & store in `kbfd_section` static
    // NB: dynamically allocated so corresponding `kbfd_section` occurs
    //     after actual data sections, as appears is customary
    object.symtab_p = new kbfd::ks_symbol(object);
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
            add_sym(s);     // `kbfd_stream` local method
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
auto kbfd_stream::core2ks_data(core::core_section const& s) const -> kbfd::ks_data& 
{
    // retrieve `ks_data *` using callback (if previously created)
    auto p = static_cast<kbfd::ks_data *>(s.kbfd_callback());
    
    if (!p)
    {
        // construct new `ks_data` section in `kbfd_object` from `core_section`
        //p = new kbfd::ks_data(object, s.sh_type, s.sh_name, s.kas_align);
        p = new kbfd::ks_data(object, s.sh_type, s.sh_name, 4);
        p->s_header.sh_flags = s.sh_flags;      // R/W, EXEC, ALLOC, etc

        // XXX group/linkage?
        // XXX st_link, st_info

        p->set_size(s.size());      // allocate memory to hold section data
        s.set_kbfd_callback(p);      // register callback
    }
    return *p;
}

// binutils defn uses `bfd_vma`. override with the ELF standard defn
#undef  ELF64_R_INFO
#define ELF64_R_INFO(s,t)   (((s) << 32) + (t))

// actually emit a reloc
// Generate an ELF64 relocation & then later to target format
void kbfd_stream::put_kbfd_reloc(core::e_chan_num num
                , kbfd::kas_reloc_info const& info 
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
    
    if (!use_rel_a)
    {
        ks_data_p->put_reloc(info, sym_num, offset);
    }
    else
    {
        ks_data_p->put_reloc_a(info, sym_num, offset, data);
        data = 0;
    }
}

}

#endif
