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
    auto save_expr = [&](auto size) -> bool
        {
            // suppress writes of zero size or zero value
            auto p = expr.get_fixed_p();
            if ((p && !*p) || !size)
            {
                info_p->has_data = false;   // supress write 
                return false;               // and no expression.
            }
            info_p->has_data = true;    
            return !inserter(std::move(expr), size);
        };
    
    // here the `has_reg` bit may be set spuriously
    // (happens when no appropriate validator present)
    // don't save if no register present
    if (!reg_p)
        info_p->has_reg = false;
    if (info_p->has_reg)
        inserter(std::move(*reg_p));
    
    // if completely saved, return `has_expr == false`
    if (!info_p->has_data)
        return false;

    // if `REG_INDIR`, save `indirect`
    if (mode() == MODE_REG_INDIR)
        inserter(indir.value(), sizeof(indir));

    // if `SHIFT` present, save
    if (mode() == MODE_SHIFT || indir.flags & arm_indirect::S_FLAG)
        inserter(shift.value(), sizeof(shift.value()));

    // needs to be `typedef`
    // XXX zero ??
    return save_expr(sizeof(uint32_t));
}


// deserialize arm_ments: for format, see above
template <typename Reader, typename ARG_INFO>
void arm_arg_t::extract(Reader& reader, uint8_t sz, ARG_INFO const *info_p)
{
    if (info_p->has_reg)
    {
        // register stored as expression
        auto p = reader.get_expr().template get_p<arm_reg_t>();
        if (!p)
            throw std::logic_error{"arm_arg_t::extract: has_reg"};
        reg_p = p;
    } 

    if (!info_p->has_data)
        return;

    // read `indirect`
    if (mode() == MODE_REG_INDIR)
        indir = reader.get_fixed(sizeof(indir));

    // read `shift`
    if (mode() == MODE_SHIFT || indir.flags & arm_indirect::S_FLAG)
        shift = decltype(shift)(reader.get_fixed(sizeof(shift.value()))); 

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
        if (is_signed)
            bytes = -bytes;
        expr = reader.get_fixed(bytes);
    }
}

}


#endif

