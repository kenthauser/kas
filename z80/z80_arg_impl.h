#ifndef KAS_Z80_Z80_ARG_IMPL_H
#define KAS_Z80_Z80_ARG_IMPL_H

#include "z80_arg.h"
#include "z80_error_messages.h"
#include "kas_core/core_emit.h"

// XXX
#include "expr/format_ieee754_impl.h"

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

    auto set_prefix = [this](auto pfx)
        {
            reg_p = &z80_reg_t::find(RC_IDX, pfx);
            this->prefix = pfx;
        };
    
    bool    is_prefix {};
    uint8_t new_prefix{};

    // check for prefix based on `reg`
    // NB: prefix for HL is zero
    if (reg_p && reg_p->kind(RC_IDX) == RC_IDX)
        is_prefix = true;
    if (is_prefix)
        new_prefix = reg_p->value(RC_IDX);
  
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
            set_prefix(0xdd);
            break;
        case MODE_REG_IY:
        case MODE_REG_INDIR_IY:
        case MODE_REG_OFFSET_IY:
            set_prefix(0xfd);
            break;
        default:
            break;
    }
    
    base_t::set_mode(mode);
    return {};
}
int z80_arg_t::size(uint8_t sz, expression::expr_fits const *fits_p, bool *is_signed) const
{
    switch (mode())
    {
        case MODE_REG_IX:
        case MODE_REG_IY:
            return 0;
        // can be zero (eg "ex (sp),ix") or 1 (eg "ld a,(ix)")
        // return minimum
        case MODE_REG_INDIR_IX:
        case MODE_REG_INDIR_IY:
            return 0;
        case MODE_REG_OFFSET_IX:
        case MODE_REG_OFFSET_IY:
            return 1;
        default:
            return base_t::size(sz, fits_p, is_signed);
    }
}

template <typename OS>
void z80_arg_t::print(OS& os) const
{
    switch (mode())
    {
        case MODE_DIRECT:
        case MODE_IMMED_QUICK:
            os << std::dec << expr;
            break;
        case MODE_INDIRECT:
            os << "(" << std::dec << expr << ")";
            break;
        case MODE_IMMEDIATE:
            os << "#" << std::dec << expr;
            break;
        case MODE_REG:
        case MODE_REG_IX:
        case MODE_REG_IY:
            os << *reg_p;
            break;
        case MODE_REG_INDIR:
        case MODE_REG_INDIR_IX:
        case MODE_REG_INDIR_IY:
            os << "(" << *reg_p << ")";
            break;
        case MODE_REG_OFFSET_IX:
        case MODE_REG_OFFSET_IY:
            if (auto p = get_fixed_p())
                if (*p < 0)
                {
                    os << "(" << *reg_p << std::dec << *p << ")";
                    break;
                }
            os << "(" << *reg_p << "+" << std::dec << expr << ")";
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
            os << "** INVALID: mode = " << +mode() << " **";
            break;
    }
}
}

#endif

