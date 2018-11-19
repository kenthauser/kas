#ifndef KAS_ELF_ELF_CORE_SYMBOL_H
#define KAS_ELF_ELF_CORE_SYMBOL_H

#include "elf_file.h"

namespace kas::elf
{

uint32_t es_symbol::add(core::core_symbol& s)
{
    Elf64_Sym sym {};

    // copy basic attributes of core_symbol to elf_symbol
    auto st_type = s.kind();
    auto st_bind = s.binding();
    sym.st_size  = s.size();
    sym.st_other = s.visibility() & 3; 
    sym.st_name  = sym_string.put(s.name());

    // use specific encoding based on symbol type
    switch (st_type) {
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
    if (auto p = s.addr_p()) {
        // NB: es_data is friends with core_section.
        auto& data   = es_data::core2es(p->frag_p->section());
        sym.st_shndx = data.section.index();
        sym.st_value = p->frag_p->base_addr()() + (*p->offset_p)();
    } else if (auto p = s.value_p()) {
        auto fixed_p = p->get_fixed_p();
        sym.st_value = fixed_p ? *fixed_p : 0;
        sym.st_shndx = SHN_ABS;
    }

    // converted core::symbol values into `ELF`. Now construct symbol
    sym.st_info  = ELF64_ST_INFO(st_bind, st_type); 
   
    put(cvt().cvt(sym), s_header.sh_entsize);

    // return symbol number && increment symbol count
    s.sym_num(++sym_num);

    // set `sh_info` to one past last local symbol
    if (st_bind == STB_LOCAL)
        s_header.sh_info = sym_num + 1;
    
    return sym_num;
}

bool es_symbol::should_emit_local(core::core_symbol& s) const
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


bool es_symbol::should_emit_non_local(core::core_symbol& s) const
{
    // don't allow skipped LOCAL's to sneek thru
    return s.binding() != STB_LOCAL;
}



}

#endif
