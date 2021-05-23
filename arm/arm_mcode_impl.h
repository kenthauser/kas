#ifndef KAS_ARM_ARM_MCODE_IMPL_H
#define KAS_ARM_ARM_MCODE_IMPL_H

// implement `arm_mcode_t` methods overriden in CRTP derived class

#include "arm_mcode.h"

namespace kas::arm
{
auto arm_mcode_t::code(stmt_info_t stmt_info) const
    -> std::array<mcode_size_t, MAX_MCODE_WORDS>
{
    // init code array using base method
    auto code_data = base_t::code(stmt_info);
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
    return code_data;
}



}

#endif
