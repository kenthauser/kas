#ifndef KAS_Z80_Z80_ARG_IMPL_H
#define KAS_Z80_Z80_ARG_IMPL_H

#include "z80_arg.h"
#include "z80_error_messages.h"
#include "kas_core/core_emit.h"

namespace kas::z80
{

// update `prefix` based on MODE.
const char *z80_arg_t::set_mode(unsigned mode)
{
    // here translate generic mode to z80 IX/IY Modes
    auto xlate_mode = [](auto mode, auto prefix) -> unsigned
        {
            auto is_iy = prefix == 0xfd;

            if (mode == MODE_REG)
                return MODE_REG_IX + is_iy;
            if (mode == MODE_REG_INDIR)
                return MODE_REG_INDIR_IX + is_iy;
            if (mode == MODE_REG_OFFSET)
                return MODE_REG_OFFSET_IX + is_iy;
            return mode;
        };
    
    bool    is_prefix {};
    uint8_t new_prefix{};

    // check for prefix based on `reg`
    // NB: prefix for HL is zero
    if (reg.kind(RC_IDX) == RC_IDX)
        is_prefix = true;
    if (is_prefix)
        new_prefix = reg.value(RC_IDX);
  
    // insns with multiple index registers must all be same
    if (is_prefix && has_prefix && new_prefix != prefix)
        return error_msg::ERR_invalid_idx;

    // if first prefix, save in static
    if (is_prefix && !has_prefix)
    {
        has_prefix = true;
        prefix     = new_prefix;
    }
   
    // xlate IX/IY into new "modes"
    if (new_prefix)
        mode = xlate_mode(mode, new_prefix);

    // deserialize case: set prefix based on mode
    else switch (mode)
    {
        case MODE_REG_IX:
        case MODE_REG_INDIR_IX:
        case MODE_REG_OFFSET_IX:
            prefix = 0xdd;
            break;
        case MODE_REG_IY:
        case MODE_REG_INDIR_IY:
        case MODE_REG_OFFSET_IY:
            prefix = 0xfd;
            break;
        default:
            break;
    }
    
    base_t::set_mode(mode);
    return {};
}

#if 0
// sz is from validator
void z80_arg_t::emit(core::emit_base& base, uint8_t sz) const
{
    // IX/IY offsets are emitted by base code. don't double emit
    switch (mode())
    {
        default:
            break;
        
        case MODE_REG_INDIR_IX:
        case MODE_REG_INDIR_IY:
        case MODE_REG_OFFSET_IX:
        case MODE_REG_OFFSET_IY:
            return;
    }
    // XXX base_t::emit(base, sz, bytes);
}
#endif

template <typename OS>
void z80_arg_t::print(OS& os) const
{
    switch (mode())
    {
        case MODE_DIRECT:
        case MODE_IMMED_QUICK:
            os << tok;
            break;
        case MODE_INDIRECT:
            os << "(" << tok << ")";
            break;
        case MODE_IMMEDIATE:
            os << "#" << tok;
            break;
        case MODE_REG:
        case MODE_REG_IX:
        case MODE_REG_IY:
            os << reg;
            break;
        case MODE_REG_INDIR:
        case MODE_REG_INDIR_IX:
        case MODE_REG_INDIR_IY:
            os << "(" << reg << ")";
            break;
        case MODE_REG_OFFSET_IX:
        case MODE_REG_OFFSET_IY:
            // print (ix-N) iff (N == fixed && N < 0)
            // else print (ix+N)
            if (auto p = tok.get_fixed_p())
                if (*p < 0)
                {
                    os << "(" << reg << std::dec << *p << ")";
                    break;
                }
            os << "(" << reg << "+" << tok << ")";
            break;
        case MODE_ERROR:
            if (err)
                os << err;
            else
                os << "Err: *UNDEFINED*";
            break;
        case MODE_NONE:
            os << "*NONE*";
            break;
        default:
            os << "** INVALID: " << +mode() << " **";
            break;
    }
}
}

#endif

