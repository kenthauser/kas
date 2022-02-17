#ifndef KAS_ARM_ARM_ARG_IMPL_H
#define KAS_ARM_ARM_ARG_IMPL_H

// implement `arm_arg_t` methods overriden in CRTP derived class

#include "arm_arg.h"
#include "arm_stmt.h"
#include "arm_float_format.h"
#include "arm_error_messages.h"

namespace kas::arm::parser
{

const char *arm_arg_t::set_mode(unsigned mode)
{
    base_t::set_mode(mode);
    
    // implement non-generic modes
    switch (mode)
    {
        case MODE_REG_UPDATE:
            {
                // `reg_p` not set before extract
                if (reg_p && reg_p->kind(RC_GEN) != RC_GEN)
                    return "general register required";
            }
            break;
        
        default:
            break;
            
    }
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

// `shift` binary helpers
uint16_t arm_shift::arm7_value() const
{
    if (is_reg)
        return (type << 5) | (ext << 8) | 0x10;    
    return (type << 5) | ((ext & 0x1f) << 7);
}

void arm_shift::arm7_set(uint16_t value)
{
    // extract "shift" from opcode.
    is_reg = !!(value & 0x10);
    value >>= 5;
    type = value & 3;
    value >>= 2;
    value &=  0x1f;
    if (is_reg)
        ext = value >> 1;
    else 
        ext = value;
}

template <typename OS>
void arm_arg_t::print(OS& os) const
{
    std::cout << "(mode = " << std::dec << +mode() << ") ";
    switch (mode())
    {
        case MODE_DIRECT:
        case MODE_BRANCH:
        case MODE_CALL:
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

        case MODE_REGSET:
            regset_p->print(os);
            break;
        
        case MODE_REGSET_USER:
            regset_p->print(os);
            os << '^';
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
            bool plus = u_flag ^ invert;
            return plus ? "+" : "-";
        };

    //std::cout << "arm_indirect: flags = " << std::hex << +flags << std::endl;

    // for post-indexed
    if (!p_flag)
       os << "]";

    // now add offset (immed, reg, shift)
    // 1. if register format
    if (r_flag)
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
    if (p_flag)
    {
        if (w_flag)
            os << "]!";
        else
            os << "]";
    }
}
    
}

#endif

