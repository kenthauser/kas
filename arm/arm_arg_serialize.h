#ifndef KAS_ARM_ARM_ARG_SERIALIZE_H
#define KAS_ARM_ARM_ARG_SERIALIZE_H

// serialize a single argument
//
// for each argument, save the "mode", register number, and data
//
// If possible, save register number (and mode) in actual "opcode" data.
// If needed, save extra data.
// Data can be stored as "fixed" constant, or "expression" 

#include "arm_arg.h"


namespace kas::arm::parser
{

template <typename Inserter, typename ARG_INFO>
bool arm_arg_t::serialize (Inserter& inserter
                         , uint8_t sz
                         , ARG_INFO *info_p
                         , bool has_val)
{
    auto save_expr = [&](expr_t expr, uint8_t bytes) -> bool
        {
            // suppress writes of zero size or zero value
            auto p = expr.get_fixed_p();
            if ((p && !*p) || !bytes)
            {
                info_p->has_data = false;   // suppress write 
                return false;               // and no expression.
            }
            info_p->has_data = true;    
            return !inserter(std::move(expr), bytes);
        };
   
    // if no validator, serialize values normally in `opcode` via inserter
    if (!has_val)
    {
        info_p->has_reg = bool(reg_p);
        if (info_p->has_reg)
            inserter(reg_p->index());

        // serialize information not stored in opcodes
        switch (mode())
        {
            case MODE_REG_INDIR:
            case MODE_SHIFT:
            {
                auto value = (indir.value() << 8) | shift.value();
                inserter(value, 2);     // always save 2 bytes
                break;
            }
            default:
                break;
        }
    }

    // regset values are never stored in opcode
    if (mode() == MODE_REGSET)
    {
                std::cout << "inserter: REGSET index = " << +regset_p->index() << std::endl;
                inserter(regset_p->index(), 2);
    }
    
    // if `expression` save it
    return save_expr(expr, 2);
}


// deserialize arguments: for format, see above
template <typename Reader, typename ARG_INFO>
void arm_arg_t::extract(Reader& reader
                      , uint8_t sz
                      , ARG_INFO const *info_p
                      , bool has_val)
{
    // if no validator, restore values normally in `opcode` from `store`
    if (!has_val)
    {
        if (info_p->has_reg)
        {
            // register stored as index
            auto reg_idx = reader.get_fixed(sizeof(typename reg_t::reg_name_idx_t));
            reg_p = &reg_t::get(reg_idx);
        } 
    
        switch (mode())
        {
            case MODE_REG_INDIR:
            case MODE_SHIFT:
            {
                auto value = reader.get_fixed(2);
                shift = value;
                indir = value >> 8;
            }
            default:
                break;
        }
    }

    if (mode() == MODE_REGSET)
    {
        auto rs_idx = reader.get_fixed(2);
        std::cout << "extract: REGSET index = " << +rs_idx << std::endl;
        regset_p = &regset_t::get(rs_idx);
    }

    // if no `expr`, return
    if (!info_p->has_data)
        return;

    // read expression. Check for register
    if (info_p->has_expr)
    {
        expr = reader.get_expr();

        // if expression holds register, process
        if (auto p = expr.template get_p<arm_reg_t>())
        {
            reg_p = p;
            expr = {};
        }
    }

    // here has data, but not expression
    else if (info_p->has_data)
    {
        bool is_signed {false};
        int  bytes = this->size(sz, {}, &is_signed);
        bytes = 2;
        if (is_signed)
            bytes = -bytes;
        expr = reader.get_fixed(bytes);
    }
}

}


#endif

