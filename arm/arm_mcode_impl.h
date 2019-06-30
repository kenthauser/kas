#ifndef KAS_ARM_ARM_MCODE_IMPL_H
#define KAS_ARM_ARM_MCODE_IMPL_H

#include "arm_stmt.h"
#include "target/tgt_mcode_defn.h"
#include "kas_core/core_emit.h"
#include "expr/expr_fits.h"

namespace kas::arm
{

auto arm_mcode_t::code(unsigned stmt_flags) const
    -> std::array<mcode_size_t, MAX_MCODE_WORDS>
{
    // put base code in array
    auto code_data = base_t::code(stmt_flags);
    
    // add ccode & s_flags as appropriate
    auto sz = this->sz();
    arm_stmt_t::flags_t flags(stmt_flags);
    
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

    std::cout << "mcode_t::code: " << std::hex << stmt_flags << std::endl;
    std::cout << "mcode_t::code: " << std::hex << flags.value() << " " << +sz;
    std::cout << " -> " << high_word << std::endl;

    if (high_word)
        code_data[0] |= high_word << 16;
    
    return code_data;
}


}

#endif
