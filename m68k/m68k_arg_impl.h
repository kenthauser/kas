#ifndef KAS_M68K_M68K_ARG_IMPL_H
#define KAS_M68K_M68K_ARG_IMPL_H


#include "m68k_arg.h"
#include "m68k_stmt.h"
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

// calculate size of serialized constant data
int8_t m68k_arg_t::serial_data_size(uint8_t sz) const
{ 
    // 
    switch (mode())
    {
        // default: two-byte unsigned if non-zero expression
        default:
            return 2;       // if they have non-zero expression

        // declare the modes with word offsets
        case MODE_ADDR_DISP:
        case MODE_PC_DISP:
        case MODE_MOVEP:
            return -2;
        case MODE_ADDR_DISP_LONG:
        case MODE_PC_DISP_LONG:
            return -4;
        case MODE_DIRECT:
        case MODE_DIRECT_ALTER:
        case MODE_DIRECT_PCREL:
        case MODE_DIRECT_LONG:
            return 4;
        case MODE_IMMEDIATE:
            return -immed_info(sz).sz_bytes;
        
        case MODE_IMMED_QUICK:
        case MODE_REG_QUICK:
            return 0;
    }
}


auto m68k_arg_t::ok_for_target(void const *stmt_p) -> kas::parser::kas_error_t
{
    auto error = [this](const char *msg)
        {
            set_mode(MODE_ERROR);
            return err = kas::parser::kas_diag_t::error(msg, *this).ref();
        };

    // get reference to current stmt
    auto& stmt = *static_cast<m68k_stmt_t const *>(stmt_p);
    auto  sz   = stmt.info.arg_size;
    auto _mode = mode();

    // 0. perform generic checks
    if (auto result = base_t::ok_for_target(stmt_p))
        return result;

    // 1. can't access address register as byte
    if (_mode == MODE_ADDR_REG && sz == OP_SIZE_BYTE)
       return error(error_msg::ERR_addr_reg_byte);

    // 2. floating point: allow register direct for 32-bit & under `source size`
    if (_mode <= MODE_ADDR_REG)
        if (immed_info(sz).sz_bytes > 4)
            return error(error_msg::ERR_direct);

    // 3. disable several addressing modes on coldfire FPUs
    if (sz == OP_SIZE_XTND)
        if (auto msg = hw::cpu_defs[hw::fpu_x_addr()])
            return error(msg);

    if (sz == OP_SIZE_PACKED)
        if (auto msg = hw::cpu_defs[hw::fpu_p_addr()])
            return error(msg);
#if 0
    // XXX are these already precluded by 3-word rule?
    // 4. floating point: iff coldfire, restrict modes
    if (info.is_cp(hw::fpu{}) && !(*reg_t::hw_cpu_p)[hw::coldfire{}])
    {
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

    // 5. disallow "SUBWORD" (coldfire MAC) except on coldfire with `word` format
    if (has_subword_mask
        || _mode == MODE_SUBWORD_LOWER
        || _mode == MODE_SUBWORD_UPPER)
    {

        if (auto err = hw::cpu_defs[hw::coldfire{}])
            return error(error_msg::ERR_subreg);
        if (_mode > MODE_ADDR_REG && sz != OP_SIZE_WORD)
            return error("X subreg for long instruction");
    }

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
    case MODE_PC_DISP_LONG:
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
            if (e.template get_p<m68k_reg_set_t>())
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

