#ifndef KAS_ARM_ARM_ARG_IMPL_H
#define KAS_ARM_ARM_ARG_IMPL_H

// implement `arm_arg_t` methods overriden in CRTP derived class

#include "arm_arg.h"
#include "arm_stmt.h"
#include "arm_float_format.h"
#include "arm_error_messages.h"

namespace kas::arm
{

const char *arm_arg_t::set_mode(unsigned mode)
{
    // implement non-generic modes
    switch (mode)
    {
        case MODE_REG_UPDATE:
            {
#ifdef XXX
                reg_t *reg_p = expr.template get_p<reg_t>();
                if (reg_p && reg_p->kind(RC_GEN) == RC_GEN)
                {
                    reg = *reg_p;
                    expr = {};
                }
                else
                    return "general register required";
#else
                if (!reg_p || reg_p->kind(RC_GEN) != RC_GEN)
                    return "general register required";
#endif
            }
            break;
        
        default:
            break;
            
    }

    base_t::set_mode(mode);
    return {};
}

bool arm_arg_t::is_immed() const
{
    {
        switch (mode())
        {
            case MODE_IMMEDIATE:
            case MODE_IMMED_QUICK:
            case MODE_IMMED_LOWER:     // :lower:
            case MODE_IMMED_UPPER:     // :upper:
            case MODE_IMMED_BYTE_0:    // :lower0_7:#
            case MODE_IMMED_BYTE_1:    // :lower8_15:#
            case MODE_IMMED_BYTE_2:    // :upper0_7:#
            case MODE_IMMED_BYTE_3:    // :upper8_15:#
                return true;
            default:
                return false;
        }
    }
}


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

        // parts of immed
        case MODE_IMMED_LOWER:
            os << "#:lower:" << expr; break;
        case MODE_IMMED_UPPER:
            os << "#:upper:" << expr; break;
        case MODE_IMMED_BYTE_0:
            os << "#:lower0_7:#" << expr; break;
        case MODE_IMMED_BYTE_1:
            os << "#:lower8_15:#" << expr; break;
        case MODE_IMMED_BYTE_2:
            os << "#:upper0_7:#" << expr; break;
        case MODE_IMMED_BYTE_3:
            os << "#:upper8_15:#" << expr; break;
        
        case MODE_REG:
            os << *reg_p;
            break;
        
        case MODE_REG_UPDATE:
            os << *reg_p << "!";
            break;

        case MODE_REG_INDIR:
            os << "[" << *reg_p;
            indir.print(os, *this);
            break;

        case MODE_SHIFT:
            shift.print(os);
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
            os << "** INVALID: ** MODE=" << +mode() << " **";
            break;
    }   
}

template <typename OS>
void arm_shift::print(OS& os) const
{
    static constexpr const char *names[] = {"LSL", "LSR", "ASR", "ROR", "RRX"};

    // test for register SHIFT
    if (is_reg)
        os << names[type] << " " << arm_reg_t::find(RC_GEN, ext);

    // test for RRX special case (looks like `ROR #0`)
    else if (type == 3 && ext == 0)
        os << names[4];

    // Immediate SHIFT
    else
    {
        // LSR & ASR map count of 0 to count = 32
        auto count = ext;
        if (type != 0 && count == 0)
            count = 32;
        os << names[type] << " #" << +count;
    }
 }
 
template <typename OS>
void arm_indirect::print(OS& os, arm_arg_t const& arg) const
{
    auto get_sign = [this](bool invert = false) -> const char *
        {
            bool minus = !(flags & U_FLAG) ^ invert;
            return minus ? "-" : "+";
        };

    //std::cout << "arm_indirect: flags = " << std::hex << +flags << std::endl;

    // for post-increment
    if ((flags & P_FLAG) == 0)
       os << "]";

    // now add offset (immed, reg, shift)
    // 1. if register format
    if (flags & R_FLAG)
    {
        os << ", " << get_sign() << arm_reg_t::find(RC_GEN, reg);
            
        if (arg.shift)
        {
            os << ", ";
            arg.shift.print(os);
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
        if (p && value)
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

