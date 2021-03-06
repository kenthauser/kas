#ifndef KAS_CORE_EMIT_KBFD_IMPL_H
#define KAS_CORE_EMIT_KBFD_IMPL_H

#include "emit_kbfd.h"

#include "kbfd/kbfd_section_sym.h"
#include "kbfd/kbfd_section_data.h"

namespace kas::core
{

// INIT `kbfd` with sections & symbols from `kas_core`
void emit_kbfd::open() 
{
    auto& kbfd = *kbfd_p;

    // 1. allocate `data_section` memory for maximum number of sections
    // NB: as `kbfd_object` uses std::vector to hold pointers to sections,
    // NB: relatively cheap to resize vector. This step may be omitted.

    // allow for creation of 2 relocation sections for each data section
    // allow for "global" sections: eg .symtab, .strtab, .shstrtab
    auto SECTION_RESERVE_CNT  = 5;       // for global sections...
         SECTION_RESERVE_CNT += core::core_section::num_objects() * 3; 

    kbfd.reserve_sections(SECTION_RESERVE_CNT);

    // 2. create a ELF_SECTION for each core_section which is not empty
    core::core_section::for_each([&](auto& s)
        { 
            if (s.size())
                core2ks_data(s);   // creates associated kbfd_section
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
    kbfd.symtab_p = new kbfd::ks_symbol(kbfd);
    kbfd.symtab_p->reserve(n_syms);          // reserve entries

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
            if (s.size())       // if section is not empty
                add_sym(s);     // `emit_kbfd` local method
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
 
// write object data to stream
void emit_kbfd::close()
{
    kbfd_p->write(out);
}


// construct new ELF data section from `core::core_section` section
auto emit_kbfd::core2ks_data(core::core_section const& s) const
                -> kbfd::ks_data& 
{
    // retrieve `ks_data *` using callback (if previously created)
    auto p = static_cast<kbfd::ks_data *>(s.kbfd_callback());

    // allocate new `callback` if required
    if (!p)
    {
        // construct new `ks_data` section in `kbfd_object` from `core_section`
        //p = new kbfd::ks_data(object, s.sh_type, s.sh_name, s.kas_align);
        p = new kbfd::ks_data(*kbfd_p, s.sh_type, s.sh_name, 4);
        p->s_header.sh_flags = s.sh_flags;      // R/W, EXEC, ALLOC, etc

        // XXX group/linkage?
        // XXX st_link, st_info

        p->set_size(s.size());      // allocate memory to hold section data
        s.set_kbfd_callback(p);     // register callback
    }
    return *p;
}

//
// kbfd data section methods
//
void emit_kbfd::put_uint(e_chan_num num, uint8_t width, int64_t data)
{
    if (do_emit(num))
        ks_data_p->put_int(data, width);
}

void emit_kbfd::put_raw(e_chan_num num
            , void const *data_p
            , uint8_t chunk_size
            , unsigned num_chunks)
{
    if (do_emit(num))
        ks_data_p->put_raw(data_p, chunk_size * num_chunks);
}

void emit_kbfd::put_data(e_chan_num num
            , void const *data_p
            , uint8_t chunk_size
            , unsigned num_chunks)
{
    if (do_emit(num))
        switch(chunk_size)
        {
        case 0:
            // short circuit null case
            break;

        case 1:
            // short circut simple (byte) case
            // NB: think ascii strings
            ks_data_p->put_raw(data_p, num_chunks);
            break;

        default:
            // byte swap while emitting
            ks_data_p->put_data(data_p, chunk_size, num_chunks);
        }
}

void emit_kbfd::put_diag(e_chan_num num, uint8_t width, parser::kas_diag_t const& diag) 
{
    static constexpr char zero[8] = {};

    // emit `width` of zeros
    if (do_emit(num))
        ks_data_p->put_raw(zero, width);
}

std::size_t emit_kbfd::position() const 
{
    return ks_data_p->position();
}

// put opaque memory block into data segment
void emit_kbfd::put(e_chan_num num, void const *p, uint8_t width)
{
    if (num == EMIT_DATA)
        ks_data_p->put(p, width);
}

//
// actually emit a reloc
//

void emit_kbfd::put_symbol_reloc(
              e_chan_num num
            , kbfd::kbfd_target_reloc const& info
            , core_symbol_t const& sym
            , int64_t  addend
            , bool use_rela
            ) 
{
    auto sym_num = sym.sym_num();
    if (!sym_num)
        throw std::logic_error("emit_kbfd: no sym_num for symbol: " + sym.name());
    put_kbfd_reloc(num, info, sym_num, addend);
}

// section reloc: lookup section as symbol #, then trampoline...
void emit_kbfd::put_section_reloc(
              e_chan_num num
            , kbfd::kbfd_target_reloc const& info
            , core_section const *section_p
            , int64_t addend 
            , bool use_rela
            ) 
{
    // assume `symbol_t` has correct type for `sym_num`
    decltype(std::declval<core_symbol_t>().sym_num()) sym_num{};
    if (section_p)
        sym_num = core2ks_data(*section_p).sym_num;
    put_kbfd_reloc(num, info, sym_num, addend);
}
    
void emit_kbfd::put_kbfd_reloc(
                  e_chan_num num
                , kbfd::kbfd_target_reloc const& info 
                , uint32_t sym_num
                , int64_t  addend
                , bool     use_rela
                ) const
{
    if (use_rela)
        ks_data_p->put_reloc_a(info, sym_num, addend);
    else
        ks_data_p->put_reloc(info, sym_num);
}
}

#endif
