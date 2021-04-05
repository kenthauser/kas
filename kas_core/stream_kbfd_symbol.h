#ifndef KAS_CORE_STREAM_KBFD_SYMBOL_H
#define KAS_CORE_STREAM_KBFD_SYMBOL_H

//#include "kbfd_file.h"

namespace kas::core
{

void kbfd_stream::add_sym(core_symbol_t& s) const
{
    // generate a ELF host format symbol
    kbfd::Elf64_Sym sym {};

    // copy basic attributes of core_symbol to kbfd_symbol
    auto st_type = s.kind();
    auto st_bind = s.binding();
    sym.st_size  = s.size();
    sym.st_other = s.visibility() & 3; 

    // use specific encoding based on symbol type
    switch (st_type)
    {
        default:
            break;
        case STT_FILE:
            sym.st_value = 0;
            sym.st_size  = 0;
            sym.st_shndx = SHN_ABS;
            break;
        case STT_COMMON:
            sym.st_value = s.align();
            sym.st_shndx = SHN_COMMON;
            break;
    }

    // if converted local COMMON, encode as STT_OBJECT
    if (st_type == STT_NOTYPE && sym.st_size)
        st_type = STT_OBJECT;

    // XXX not correct for value_p != ABS
    // XXX value_p can hold any `expr_t` value, not just FIXED & ADDR
    if (auto p = s.addr_p())
    {
        // NB: ks_data is friends with core_section.
        auto& data   = core2ks_data(p->frag_p->section());
        sym.st_shndx = data.index;
        sym.st_value = p->frag_p->base_addr()() + (*p->offset_p)();
    } 
    else if (auto p = s.value_p())
    {
        auto fixed_p = p->get_fixed_p();
        sym.st_value = fixed_p ? *fixed_p : 0;
        sym.st_shndx = SHN_ABS;
    }

    sym.st_info  = ELF64_ST_INFO(st_bind, st_type); 

    // converted core::symbol values into `ELF`. Now insert symbol
    s.set_sym_num(kbfd_p->symtab_p->add(sym, s.name()));
}

// create a `STT_SECTION` symbol for a data section
void kbfd_stream::add_sym(core_section& s) const
{
    // get reference to corresponding ELF data section
    auto& data = core2ks_data(s);

    // create symbol & initialize non-default fields 
    kbfd::Elf64_Sym sym{};

    sym.st_info  = ELF64_ST_INFO(STB_LOCAL, STT_SECTION);
    sym.st_shndx = data.index;

    // NB: name is convenience for debugging (and may be omitted)
    data.sym_num = kbfd_p->symtab_p->add(sym, data.name);
}


bool kbfd_stream::should_emit_local(core_symbol_t& s) const
{
    // suppress symbols with "Local" names
    bool suppress_local_names = true;

    if (suppress_local_names) {
        switch (s.name().front())
        {
            default:
                break;
            case '.':
            case 'L':
                return false;
        }
    }

    return true;
}


bool kbfd_stream::should_emit_non_local(core_symbol_t& s) const
{
    // don't allow skipped LOCAL's to sneek thru
    return s.binding() != STB_LOCAL;
}



}

#endif
