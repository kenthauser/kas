#ifndef KAS_M68K_M68K_ARG_SERIALIZE_H
#define KAS_M68K_M68K_ARG_SERIALIZE_H

// serialize a single argument
//
// for each argument, save the "mode", register number, and data
//
// If possible, save register number (and mode) in actual "opcode" data.
// If needed, save extra data.
// Data can be stored as "fixed" constant, or "expression" 

#include "m68k_arg.h"


namespace kas::m68k
{

template <typename Inserter, typename ARG_INFO>
bool m68k_arg_t::serialize (Inserter& inserter
                          , uint8_t sz
                          , ARG_INFO *serial_p
                          , bool has_val)
{
    auto save_expr = [&](auto expr, uint8_t bytes, uint8_t flt_fmt = {}) -> bool
        {
            // suppress writes of zero
            auto p = expr.get_fixed_p();
            if (p && !*p)
            {
                serial_p->has_data = false;    
                return false;               // and no expression.
            }
            
            // non-zero expression. must save.
            serial_p->has_data = true;    
           #if 0 
            auto& arg_info = m68k_arg_t::sz_info[sz];
            auto  bytes    = arg_info.sz_bytes;
            auto  flt_fmt  = arg_info.flt_fmt;
           #endif
            if (p && flt_fmt)
                expr = e_float_t::add(*p);

            // if longer than LONG, save as expr
            else if (bytes > 4)
                bytes = 0;

            // return true iff expression, not fixed
            return !inserter(std::move(expr), bytes);
        };
    
    // if no validator, serialize values normally in `opcode` via inserter
    if (!has_val)
    {
        serial_p->has_reg = bool(reg_p);
        if (serial_p->has_reg)
            inserter(reg_p->index());

        // pick up MODE_IMMED_QUICK in general switch
    }
#if 0
    // don't save if no register present
    if (serial_p->has_reg || mode() != MODE_IMMED_QUICK)
        serial_p->has_data = true;
    
    if (!reg_p)
        serial_p->has_reg = false;
    if (serial_p->has_reg)
        inserter(reg_p->index());
#endif
    // need to special case modes with data
    switch (mode())
    {
        case MODE_INDEX:
        case MODE_PC_INDEX:
        {
            auto size = 0;
            
            // 1) always save "extension" word
            inserter(ext.value(), 2);

            // 2) use "has_expr" bit as inner_has_expression
            switch (ext.disp_size)
            {
                case M_SIZE_ZERO:
                    size = 1;       // flag
                    break;
                case M_SIZE_AUTO:
                    size = 0;
                    break;
                case M_SIZE_WORD:
                    size = 2;
                    break;
                case M_SIZE_LONG:
                    size = 4;
                    break;
            }
           
            // handle "ZERO"
            if (size != 1)
                serial_p->has_expr = save_expr(expr, size);
            
            // 3) use "has_data" bit as outer_has_expression
            serial_p->has_data = false;   // could have been set by `save_expr`
            if (ext.has_outer)
            {
                switch (ext.mem_size)
                {
                    case M_SIZE_ZERO:
                    // Inner Zero -- just return
                        size = 1;       // flag
                        break;
                    case M_SIZE_AUTO:
                        size = 0;
                        break;
                    case M_SIZE_WORD:
                        size = 2;
                        break;
                    case M_SIZE_LONG:
                        size = 4;
                        break;
                }
                if (size != 1)
                    serial_p->has_data = save_expr(outer, size);
            }
            
            // serialize expressions if inner or outer unresolved
            return serial_p->has_expr || serial_p->has_data;
        }

        case MODE_ADDR_DISP:
        case MODE_PC_DISP:
        case MODE_MOVEP:
            return save_expr(expr, 2);

        case MODE_REGSET:
            // XXX why does ARM not flag this as "data"
            inserter(regset_p->index(), 2);
            break;
    
        case MODE_IMMED_QUICK:
            if (!has_val)
                return save_expr(expr, 2);      // save as 16-bit immed
            break;

        case MODE_BITFIELD:
            // XXX don't know
        default:
            {
                auto& arg_info = m68k_arg_t::sz_info[sz];
                auto  bytes    = arg_info.sz_bytes;
                auto  flt_fmt  = arg_info.flt_fmt;
                return save_expr(expr, bytes, flt_fmt);
            }
    }

    // didn't save expression
    return false;
}


// deserialize m68k arguments: for format, see above
// save pointer `extension_t` word if present
template <typename Reader, typename ARG_INFO>
void m68k_arg_t::extract(Reader& reader
                      , uint8_t sz
                      , ARG_INFO const *serial_p
                      , bool has_val)
{
    using reg_tok = meta::_t<expression::token_t<reg_t>>;

#ifdef TRACE_ARG_SERIALIZE
    std::cout << "m68k_arg_t::extract: sz = " << +sz;
    std::cout << ", has_val = " << std::boolalpha << has_val;
    std::cout << std::endl;
#endif

    if (serial_p->has_reg)
    {
        // register stored as index
        auto reg_idx = reader.get_fixed(sizeof(typename reg_t::reg_name_idx_t));
        reg_p = &reg_t::get(reg_idx);
    } 
    
    // need match `switch` in serialize above
    if (mode() == MODE_INDEX || mode() == MODE_PC_INDEX)
    {
        auto size = 0;

        // get pointer extension word & save as write-back pointer
        wb_ext_p = reader.get_fixed_p(2);
        ext = *wb_ext_p;              // init extension with saved value

        // get inner expression if stored
        if (serial_p->has_expr)       // `has_expr` mapped to `inner_has_expr`
            expr = reader.get_expr();
        else
        {
            switch (ext.disp_size)
            {
                case M_SIZE_ZERO:
                    size = 0;       // Flag ZERO for extract
                    break;
                case M_SIZE_AUTO:
                    size = 0;       // AUTO stored as expr, retrieved above
                    break;
                case M_SIZE_WORD:
                    size = -2;      // signed
                    break;
                case M_SIZE_LONG:
                    size = -4;
                    break;
            }
            
            if (size)
                expr = reader.get_fixed(size);
        }

        // get outer expression if stored
        if (serial_p->has_data)       // `has_data` mapped to `outer_has_expr`
            outer = reader.get_expr();
        else if (ext.has_outer)
        {
            switch (ext.mem_size)
            {
                case M_SIZE_ZERO:
                    size = 0;       // Flag ZERO for extract
                    break;
                case M_SIZE_AUTO:
                    size = 0;       // AUTO stored as expr, retrieved above
                    break;
                case M_SIZE_WORD:
                    size = -2;      // signed
                    break;
                case M_SIZE_LONG:
                    size = -4;
                    break;
            }
            
            if (size)
                outer = reader.get_fixed(size);
        }
    }

    else if (mode() == MODE_REGSET)
    {
        auto rs_idx = reader.get_fixed(2);
        regset_p = &regset_t::get(rs_idx);
    }

    else if (serial_p->has_expr)
    {
        expr = reader.get_expr();
    }
    else if (serial_p->has_data)
    {
        auto immed_size = m68k_arg_t::immed_info(sz).sz_bytes;

        // NB: some formats only save 16-bits regardless of `sz`
        switch (mode())
        {
            case MODE_ADDR_DISP:
            case MODE_PC_DISP:
            case MODE_MOVEP:
            case MODE_IMMED_QUICK:
                immed_size = 2;     // 16-bits only
            default:
                break;      // read immed value
        }
        expr = reader.get_fixed(immed_size);
    }
}

}
#endif

