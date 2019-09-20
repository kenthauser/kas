#ifndef KAS_M68K_M68K_ARG_IMPL_H
#define KAS_M68K_M68K_ARG_IMPL_H


#include "m68k_arg.h"
#include "m68k_format_float.h"
#include "m68k_error_messages.h"

namespace kas::m68k
{

// declare size of immed args
// NB: definition of arg modes (OP_SIZE_*) is in `m68k_mcode.h`
const tgt::tgt_immed_info m68k_arg_t::sz_info [] =
    {
          {  4  }                                       // 0: LONG
        , {  4, m68k_format_float::FMT_IEEE_32_SINGLE } // 1: SINGLE
        , { 12, m68k_format_float::FMT_M68K_80_EXTEND } // 2: XTND
        , { 12, m68k_format_float::FMT_M68K_80_PACKED } // 3: PACKED
        , {  2  }                                       // 4: WORD
        , {  8, m68k_format_float::FMT_IEEE_64_DOUBLE } // 5: DOUBLE
        , {  2, {}, 1 }                                 // 6: BYTE
        , {  0  }                                       // 7: VOID
    };

auto m68k_arg_t::ok_for_target(m68k_stmt_info_t const& info) -> kas::parser::kas_error_t
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
    auto sz = info.sz();

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
            case MODE_IMMEDIATE:
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

m68k_arg_mode m68k_arg_t::mode_normalize() const
{
    // "normalize" some modes for output in opcode
    switch (mode()) {
    default:
        return mode();
    case MODE_MOVEP:
        return MODE_ADDR_DISP;
    case MODE_ADDR_DISP_LONG:
        return MODE_INDEX;
    case MODE_DIRECT:
    case MODE_DIRECT_ALTER:
        return MODE_DIRECT_LONG;
    case MODE_DIRECT_PCREL:
        return MODE_PC_DISP;
    }
}

uint8_t m68k_arg_t::cpu_mode() const
{
    auto normalized = mode_normalize();
    if (normalized < 7)
        return normalized;
    if (normalized <= MODE_IMMEDIATE)
        return 7;
    return -1;
}

uint8_t m68k_arg_t::cpu_reg() const
{
    // special case "artificial" modes
    switch (mode())
    {
        case MODE_DIRECT:
        case MODE_DIRECT_ALTER:
            return 1;       // DIRECT_LONG
        case MODE_DIRECT_PCREL:
            return 2;       // PC_DISP
        default:
            break;
    }
    
    auto normalized = mode_normalize();
    if (normalized < 7)
        return reg_num;
    if (normalized <= MODE_IMMEDIATE)
        return normalized - 7;
    return 0;
}

// true if all `disp` and `outer` are registers or constants 
bool m68k_arg_t::is_const () const
{
    auto do_const = [](auto const& e) -> bool
        {
            // `is_const` implies insn ready to emit.
            if (e.template get_p<m68k_reg_t>())
                return true;
            if (e.template get_p<m68k_reg_set>())
                return true;
            if (e.get_fixed_p())
                return true;
            // if (e.template get_p<e_float_t>())
            //     return true;
            return false;
        };
    return do_const(expr) && do_const(outer);
}

    
}

#endif

