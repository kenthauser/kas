#ifndef KAS_ARM_ARM_MCODE_IMPL_H
#define KAS_ARM_ARM_MCODE_IMPL_H

// implement `arm_mcode_t` methods overriden in CRTP derived class

// Status bits in ARM:
//
// 1. condition-code: everywhere...
//
// 2. addressing mode 1: only S_BIT (and Immed_bit)
//     NB: 32-bit immed (means no U-bit)
//
// 3. addressing mode 2: P/U/B/W/L (P=0/W=1 -> T)
//
// 4. addressing mode 3: P/U/W/L. Also SH-> bits 6&5 differ on L/S bit
//
// 5. addressing mode 4: P/U/S/W/L: P/U set from SFX differ on L/S bit
//
// 6. addressing mode 5: (coprocessor): P/U/N/W/L (N sits at 22 (s-bit)
//                                                  and depends on co-processor)
//
// Status bits in T16:
//
// 1. ccode shifted 8 for branch. 
//




#include "arm_mcode.h"
#include "arm_info_impl.h"

namespace kas::arm
{
auto arm_mcode_t::code(stmt_info_t info) const
    -> std::array<mcode_size_t, MAX_MCODE_WORDS>
{
    auto& defn_info = defn().info;
 //   std::cout << "mcode_t::ccode: fn_idx = " << +defn_info.fn_idx << std::endl;
    
    // init code array using base method
    auto code_data = base_t::code(info);
    
#if 1
    //code_data[0] = info.value() << 16;
    //arm_info_list data;
    //data.insert(code_data, info, defn_info);
    //info_fns[defn_info.fn_idx]->insert(code_data, info, defn_info);
    defn().info_fns_base[defn_info.fn_idx]->insert(code_data, info, defn_info);
#else

    code_data[0] |= ccode << 28;

    // process s-flg
    if (stmt_info.has_sflag)
        code_data[0] |= 1 << 20;

#ifdef XXX    
    // add ccode & s_flags as appropriate
    auto sz = stmt_info.sz();
    arm_stmt_t::flags_t flags(stmt_info);
    
    // add condition code?
    unsigned high_word{};

    if (sz & SZ_DEFN_COND)
    {
        if (flags.has_ccode)
            high_word = flags.ccode << 12;
        else
            high_word = 0xe000;
    }
    if (flags.has_sflag)
        high_word |= 1 << (20-16);

    std::cout << "mcode_t::code: " << std::hex << stmt_info.value() << std::endl;
    std::cout << "mcode_t::code: " << std::hex << flags.value() << " " << +sz;
    std::cout << " -> " << high_word << std::endl;

    if (high_word)
        code_data[0] |= high_word << 16;
#endif
#endif
    return code_data;
}


auto arm_mcode_t::extract_info(mcode_size_t const *code_p) const
    -> stmt_info_t 
{
    auto& defn_info = defn().info;
//    std::cout << "mcode_t::ccode: fn_idx = " << +defn_info.fn_idx << std::endl;
    return defn().info_fns_base[defn_info.fn_idx]->extract(code_p, defn_info);
    //return info_fns[defn_info.fn_idx]->extract(code_p, defn_info);
}

}

#endif
