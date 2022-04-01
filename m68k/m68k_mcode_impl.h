#ifndef KAS_M68K_M68K_MCODE_IMPL_H
#define KAS_M68K_M68K_MCODE_IMPL_H

// implement `m68k_mcode_t` methods overriden in CRTP derived class

#include "m68k_mcode.h"

namespace kas::m68k
{
#if 1
// determine size of immediate arg
uint8_t m68k_mcode_t::sz(stmt_info_t info) const
{
    if (info.arg_size != OP_SIZE_VOID)
        return info.arg_size;

    // if void, check if single size specified
    auto defn_sz = defn().info() & 0x7f;

    // don't bother with switch
    if (defn_sz == (1 << OP_SIZE_LONG))
        return OP_SIZE_LONG;
    else if (defn_sz == (1 << OP_SIZE_WORD))
        return OP_SIZE_WORD;
    else if (defn_sz == (1 << OP_SIZE_BYTE))
        return OP_SIZE_BYTE;

    return info.arg_size;
}

#endif
#if 1
auto m68k_mcode_t::calc_branch_mode(uint8_t size) const
    -> uint8_t
{
    // calculation is arch dependent
    return arg_mode_t::MODE_BRANCH;
#if 0
    // deduce branch type from `size` & mcode
    // calculate size of displacment words 
    auto disp_size = size - base_size();

    // Assume 1 byte opcode + 1 byte displacement in single word insn
    switch (disp_size)
    {
        default:    // probably should throw...
        case 0:     // embedded in first word -> byte displacement
        case 1:     // 8-bit machine with 1 byte (8-bit) displacement
            return 0;
        case 2:     // 2 bytes => word (16-bit) displacment
            return 1;
        case 4:     // 4 bytes => long (32-bit) displacment
            return 2;
        case 8:     // 8 bytes => long long (64-bit) displacment
            return 3;
    }
#endif
}
#endif

// coldfile limit_3w is in `opc` namespace
namespace opc
{
    auto m68k_opc_base::cf_limit_3w(core::opcode_data& data) const
        -> op_size_t&
    {
        if (!hw::cpu_defs[hw::limit_3w{}])
            if (data.size.min > 6)
            {
                data.size = 6;
                data.set_error(hw::limit_3w::name::value);
            }
        
        return data.size;
    }
}

}

#endif
