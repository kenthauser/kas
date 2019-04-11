#ifndef KAS_M68K_M68K_ARG_IMPL_H
#define KAS_M68K_M68K_ARG_IMPL_H


#include "m68k_arg.h"
#include "m68k_error_messages.h"

namespace kas::m68k
{

auto m68k_arg_t::ok_for_target(uint8_t sz) -> kas::parser::kas_error_t
{
    auto error = [this](const char *msg)
        {
            set_mode(MODE_ERROR);
            return err = kas::parser::kas_diag::error(msg, *this).ref();
        };

    // 0. if parsed as error, propogate
    if (mode() == MODE_ERROR)
    {
        // if not location-tagged, use arg location
        // ie. create new "reference" from diag using `this` as loc
        if (!err.get_loc())
            err = err.get().ref(*this);
        
        return err;
    }

    // perform checks 
    // 1. can't access address register as byte
    if (mode() == MODE_ADDR_REG && sz == OP_SIZE_BYTE)
       return error(error_msg::ERR_addr_reg_byte);

    // 2. if register, make sure register supported on CPU
    if (mode() == MODE_REG)
        if (auto msg = expr.get_p<m68k_reg_t>()->validate())
            return error(msg);

    // 3. floating point: allow register direct for 32-bit & under `source size`
    if (mode() <= MODE_ADDR_REG)
        if (immed_info(sz).sz_bytes > 4)
            return error(error_msg::ERR_direct);

    // 4. disable several addressing modes on coldfire FPUs
    if (sz == OP_SIZE_XTND)
        if (auto msg = hw::cpu_defs[hw::fpu_x_addr()])
            return error(msg);

    if (sz == OP_SIZE_PACKED)
        if (auto msg = hw::cpu_defs[hw::fpu_p_addr()])
            return error(msg);
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
            return error(error_msg::ERR_subreg);

    // OK
    return {};
}

    
}

#endif

