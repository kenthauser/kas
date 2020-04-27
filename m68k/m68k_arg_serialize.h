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

template <typename Inserter, typename WB_INFO>
bool m68k_arg_t::serialize (Inserter& inserter, uint8_t sz, WB_INFO *info_p)
{
    auto save_expr = [&](auto expr, int bytes) -> bool
        {
            // if longer than LONG, save as expr
            if (bytes > 4)
                bytes = 0;

            // suppress writes of zero
            auto p = expr.get_fixed_p();
            if (p && !*p)
            {
                info_p->has_data = false;    
                return false;               // and no expression.
            }
            info_p->has_data = true;    
#if 1   
            // if possibly `e_float_t` perform tests 
            if constexpr (!std::is_void_v<e_float_t>)
            {
                std::cout << "tgt_arg_t::serialize: " << expr << std::endl;
                using fmt = typename e_float_t::object_t::fmt;
#if 1
                if (!p)
                    fmt::ok_for_fixed(expr, bytes * 8);
#endif
            }
#endif
            return !inserter(std::move(expr), bytes);
        };
    
    // here the `has_reg` bit may be set spuriously
    // (happens when no appropriate validator present)
    // don't save if no register present
    if (info_p->has_reg || mode() != MODE_IMMED_QUICK)
        info_p->has_data = true;
    
    if (!reg_p)
        info_p->has_reg = false;
    if (info_p->has_reg)
        inserter(*reg_p);
    
    switch (mode())
    {
        default:
            break;

        case MODE_INDEX:
        case MODE_PC_INDEX:
            // special case INDEX
            {
                int size;

                // 1) always save "extension" word
                inserter(ext.value(), 2);

                // 2) use "has_expr" bit as inner_has_expression
                switch (ext.disp_size)
                {
                    case M_SIZE_ZERO:
                        size = 1;       // FLAG
                        break;
                    case M_SIZE_NONE:
                        size = 0;       // AUTO
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
                    info_p->has_expr = save_expr(expr, size);
                
                // 3) use "has_data" bit as outer_has_expression
                info_p->has_data = false;   // could have been set by `save_expr`
                if (ext.outer())
                {
                    switch (ext.outer_size())
                    {
                        case M_SIZE_ZERO:
                        // Inner Zero -- just return
                            size = 1;       // FLAG
                            break;
                        case M_SIZE_NONE:
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
                        info_p->has_data = save_expr(outer, size);
                }
                
                // emit relocations if inner or outer unresolved
                return info_p->has_expr || info_p->has_data;
            }                
            break;
    }

    if (info_p->has_data)
        return save_expr(expr, size(sz).max);

    // didn't save expression
    return false;
}


// deserialize m68k arguments: for format, see above
// save pointer `extension_t` word if present
template <typename Reader>
void m68k_arg_t::extract(Reader& reader, uint8_t sz, arg_serial_t *serial_p)
{
    using reg_tok = meta::_t<expression::token_t<reg_t>>;
    
    if (serial_p->has_reg)
    {
        // register stored as index
        auto reg_idx = reader.get_fixed(sizeof(typename reg_t::reg_name_idx_t));
        reg_p = &reg_t::get(reg_idx);
    } 
    
    // need to special case INDEX to match code above
    if (mode() == MODE_INDEX || mode() == MODE_PC_INDEX)
    {
        auto size = 0;

        // get extension word
        auto& wb_ext = decltype(ext)::cast(reader.get_fixed_p(2));
        //*wb_p = &wb_ext;
        ext = wb_ext;

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
                case M_SIZE_NONE:
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
        else if (ext.outer())
        {
            switch (ext.outer_size())
            {
                case M_SIZE_ZERO:
                    size = 0;       // Flag ZERO for extract
                    break;
                case M_SIZE_NONE:
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

    else if (serial_p->has_expr)
    {
        expr = reader.get_expr();
    }
    else if (serial_p->has_data)
    {
        // get size of fixed data to read
        // NB: can't use generic `size()` because `expr` is zero
        // and `m68k_arg_t::size` converts modes freely.

        int bytes = 2;          // most are 2 bytes, unsigned
        switch (mode())
        {
            // declare the modes with word offsets
            case MODE_ADDR_DISP:
            case MODE_PC_DISP:
            case MODE_MOVEP:
                bytes = -2;
                break;
            case MODE_IMMEDIATE:
                // immed is signed
                bytes = -immed_info(sz).sz_bytes;
                break;
            case MODE_ADDR_DISP_LONG:
            case MODE_DIRECT_LONG:
                bytes = 4;
                break;
            default:
                break;
        }
        expr = reader.get_fixed(bytes);
    }
    
    // save write-back pointer to serialized data
    wb_serial_p = serial_p;
}

}


#endif
