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


namespace kas::arm
{

template <typename Inserter, typename ARG_INFO>
bool arm_arg_t::serialize (Inserter& inserter, uint8_t sz, ARG_INFO *info_p)
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
    
    // here the `has_reg` bit may be set spuriously
    // (happens when no appropriate validator present)
    // don't save if no register present
    if (!reg_p)
        info_p->has_reg = false;
    if (info_p->has_reg)
        inserter(reg_p->index());
#if 0    
    // if `REG_INDIR`, always save `indirect`
    if (mode() == MODE_REG_INDIR)
    {
        // save indir + shift as 16-bit constant
        auto value = (indir.value() << 8) | shift.value();
        inserter(value, 2);     // always save 2 bytes
    }
#endif
#if 0
    // if `SHIFT` present, always save `shift`
    if (mode() == MODE_SHIFT)
        inserter(shift.value(), 1);
#endif
    if (mode() == MODE_REGSET)
        return save_expr(regset_p->index(), 2);

    return save_expr(expr, 2);
}


// deserialize arm_ments: for format, see above
template <typename Reader, typename ARG_INFO>
void arm_arg_t::extract(Reader& reader, uint8_t sz, ARG_INFO const *info_p)
{
    if (info_p->has_reg)
    {
        // register stored as index
        auto reg_idx = reader.get_fixed(sizeof(typename reg_t::reg_name_idx_t));
        reg_p = &reg_t::get(reg_idx);
    } 
#if 0
    // for indirect, always read aux data
    if (mode() == MODE_REG_INDIR)
    {
        auto value = reader.get_fixed(2);
        shift = value;
        indir = value >> 8;
    }
#endif
#if 0
    // for shift, always read aux data
    if (mode() == MODE_SHIFT)
        shift = reader.get_fixed(1);
#endif
    if (mode() == MODE_REGSET)
    {
        auto rs_idx = reader.get_fixed(2);
        regset_p = &regset_t::get(rs_idx);
        return;
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

