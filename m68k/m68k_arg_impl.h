#ifndef KAS_M68K_M68K_ARG_IMPL_H
#define KAS_M68K_M68K_ARG_IMPL_H


#include "m68k_arg_defn.h"
#include "m68k_error_messages.h"

namespace kas::m68k
{

const char * m68k_arg_t::ok_for_target(opc::m68k_size_t sz) const
{
    // 0. if parsed as error, propogate
    if (mode == MODE_ERROR)
        return err ? err : error_msg::ERR_argument;

    // perform checks 
    // 1. can't access address register as byte
    if (mode == MODE_ADDR_REG && sz == opc::OP_SIZE_BYTE)
       return error_msg::ERR_addr_reg_byte;

    // 2. if register, make sure register supported on CPU
    if (mode == MODE_REG)
        if (auto msg = disp.get_p<m68k_reg>()->validate())
            return msg;

    // 3. floating point: allow register direct for 32-bit & under `source size`
    if (mode <= MODE_ADDR_REG)
        if (opc::m68k_size_immed[sz] > 4)
            return error_msg::ERR_direct;

    // 4. disable several addressing modes on coldfire FPUs
    if (sz == opc::OP_SIZE_XTND)
        if (auto msg = hw::cpu_defs[hw::fpu_x_addr()])
            return msg;

    if (sz == opc::OP_SIZE_PACKED)
        if (auto msg = hw::cpu_defs[hw::fpu_p_addr()])
            return msg;
#if 0
    if (info.is_cp(hw::fpu{}) && !hw::cpu_defs[hw::coldfire{}]) {
        // several addressing modes not supported by CF FPU
        // ie. immediate, index, abs_short, abs_long
        // NB: cpu_mode only set non-zero when matches addressing.
        switch (mode) {
            default:
                break;
            case MODE_INDEX:
            case MODE_PC_INDEX:
            case MODE_IMMED:
            case MODE_DIRECT_SHORT:
            case MODE_DIRECT_LONG:
                return hw::coldfire::name{};
        }
    }
#endif

    // 5. disallow "SUBWORD" (coldfire MAC) except on coldfire
    if (reg_subword != REG_SUBWORD_FULL)
        if (auto err = hw::cpu_defs[hw::coldfire{}])
            return error_msg::ERR_subreg;

    // OK
    return {};
}

    
}

#endif

