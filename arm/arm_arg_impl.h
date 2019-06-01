#ifndef KAS_ARM_ARM_ARG_IMPL_H
#define KAS_ARM_ARM_ARG_IMPL_H

#include "arm_arg.h"
#include "arm_error_messages.h"
#include "kas_core/core_emit.h"

namespace kas::arm
{

template <typename OS>
void arm_arg_t::print(OS& os) const
{
    switch (mode())
    {
        case MODE_DIRECT:
            os << expr;
            break;
        case MODE_IMMED_QUICK:
        case MODE_IMMEDIATE:
            os << "#" << expr;
            break;
        case MODE_REG:
            os << reg;
            break;
        
        case MODE_REG_UPDATE:
            os << reg << "!";
            break;

        case MODE_REG_INDIR:
            os << "[" << reg;
            indir.print(os, *this);
            break;

        case MODE_SHIFT:
            shift.print(os, *this);
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
        
        case MODE_INDIRECT:
        default:
            os << "** INVALID: " << +mode() << " **";
            break;
    }   
}

template <typename OS>
void arm_shift::print(OS& os, arm_arg_t const& arg) const
{
    static constexpr const char *names[] = {"LSL", "LSR", "ASR", "ROR", "RRX"};

    // test for RRX special case (looks like `ROR #0`)
    if (type == 3 && !is_reg && ext == 0)
        os << names[4];

    else if (is_reg)
        os << names[type] << " " << arm_reg_t{RC_GEN, reg};

    else
        os << names[type] << " #" << ext;
}

template <typename OS>
void arm_indirect::print(OS& os, arm_arg_t const& arg) const
{
    auto get_sign = [this](bool invert = false) -> const char *
        {
            bool minus = !!(flags & M_FLAG) ^ invert;
            return minus ? "-" : "+";
        };

    // for post-increment
    if ((flags & P_FLAG) == 0)
       os << "]";

    // now add offset (immed, reg, shift)
    // 1. if register format
    if (flags & R_FLAG)
    {
        os << ", " << get_sign() << arm_reg_t{RC_GEN, reg};
            
        if (arg.shift)
        {
            os << ", ";
            arg.shift.print(os, arg);
        }
    }

    // 2. if immed arg
    else
    {
        // first get sign if available
        int  value {};
        bool invert{};
        auto p = arg.expr.get_fixed_p();
        if (p)
            value = *p;

        // if constant zero, suppress `#+0`
        if (!p || value || (flags & M_FLAG))
        {
            if (value < 0)
            {
                invert = true;
                value  = -value;
            }

            // if resolved as constant, emit value
            os << ", #" << get_sign(invert);
            if (p)
                os << value;
            else
                os << arg.expr;
        }
    }

    // 3. now close if P_FLAG set
    if ((flags & P_FLAG) != 0)
    {
        if (flags & W_FLAG)
            os << "]!";
        else
            os << "]";
    }
}
}
#endif

