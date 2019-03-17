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
#include "m68k_size_defn.h"

//#include "expr/expr_fits.h"     // for INDEX_BRIEF


namespace kas::m68k
{

template <typename Inserter>
bool m68k_arg_t::serialize (Inserter& inserter, opc::m68k_size_t sz, bool& completely_saved)
{
    auto save_expr = [&](auto size) -> bool
        {
            // suppress writes of zero
            auto p = expr.get_fixed_p();
            if (p && !*p)
            {
                completely_saved = true;    // validator saved all data.
                return false;               // and no expression.
            }
            completely_saved = false;
            return !inserter(std::move(expr), size);
        };
    
    // XXX args w/o validators (list format) enter !completely_saved
    // XXX, even for base modes. May need to save reg #
    // XXX arg3 stores general reg # only (D0-A7)
    
    completely_saved = false;
    auto size = 0;
    
    switch (mode())
    {
        case MODE_IMMED_QUICK:
        case MODE_REG_QUICK:
        default:
            completely_saved = true;
            break; 

        // signed
        case MODE_ADDR_DISP:
        case MODE_PC_DISP:
        case MODE_MOVEP:
            size = -2;
            break;

        // unsigned
        case MODE_REGSET:
        case MODE_DIRECT_SHORT:
        case MODE_INDEX_BRIEF:
        case MODE_PC_INDEX_BRIEF:
            size = 2;
            break;

        // unsigned
        case MODE_DIRECT_LONG:
            size = 4;
            break;

        case MODE_REG:
        case MODE_PAIR:
        case MODE_BITFIELD:
            // save as expression
            break;

        case MODE_IMMED:
        case MODE_IMMED_LONG:
        case MODE_IMMED_SINGLE:
        case MODE_IMMED_XTND:
        case MODE_IMMED_PACKED:
        case MODE_IMMED_WORD:
        case MODE_IMMED_DOUBLE:
        case MODE_IMMED_BYTE:
            size = opc::m68k_size_immed[sz];
            if (size > 4)
                size = 0;       // longer than LONG, save as expr
            else
                size = -size;   // actually signed
            break;

        case MODE_INDEX:
        case MODE_PC_INDEX:
            // special case INDEX
            {
                // 1) always save "extension" word
                inserter(ext.value(), 2);

                // 2) use "has expression" bit for inner word
                switch (ext.disp_size)
                {
                    case M_SIZE_ZERO:
                        size = 1;       // FLAG
                        break;
                    case M_SIZE_NONE:
                        size = 0;       // AUTO
                        break;
                    case M_SIZE_WORD:
                        size = -2;
                        break;
                    case M_SIZE_LONG:
                        size = 4;
                        break;
                }
                bool inner_expr = false;
                
                // handle "ZERO"
                if (size != 1)
                    inner_expr = save_expr(size);
                
                // 3) re-use "has data" bit as outer_has_expression
                completely_saved = true;       // outer_has_expression = false
                if (!ext.outer())
                    return inner_expr;

                switch (ext.outer_size())
                {
                    case M_SIZE_ZERO:
                    // Inner Zero -- just return
                        return inner_expr;
                    case M_SIZE_NONE:
                        size = 0;
                        break;
                    case M_SIZE_WORD:
                        size = -2;
                        break;
                    case M_SIZE_LONG:
                        size = 4;
                        break;
                }
                completely_saved = inserter(std::move(outer), size);
                return inner_expr;
            }                
            break;
    }

    if (!completely_saved)
        return save_expr(size);

    // didn't save expression
    return false;
}


// deserialize m68k_ments: for format, see above
template <typename Reader>
void m68k_arg_t::extract(Reader& reader, opc::m68k_size_t sz, bool has_data, bool has_expr)
{
    // need to special case INDEX to match code above
    if (mode() == MODE_INDEX || mode() == MODE_PC_INDEX)
    {
        auto size = 0;

        // get extension word
        ext = reader.get_fixed(2);

        // get inner expression if stored
        if (has_expr)
            expr = reader.get_expr();
        else
        {
            switch (ext.disp_size)
            {
                case M_SIZE_ZERO:
                    size = 0;       // Flag is ZERO FOR extract
                    break;
                case M_SIZE_NONE:
                    size = 0;       // AUTO stored as expr, retrieved above
                    break;
                case M_SIZE_WORD:
                    size = -2;
                    break;
                case M_SIZE_LONG:
                    size = 4;
                    break;
            }
            if (size)
                expr = reader.get_fixed(size);
        }

        // get outer expression if stored
        if (has_data)
            outer = reader.get_expr();
        else if (ext.outer())
        {
            switch (ext.outer_size())
            {
                case M_SIZE_ZERO:
                    size = 0;       // Flag is ZERO for extract
                    break;
                case M_SIZE_NONE:
                    size = 0;       // AUTO stored as expr, retrieved above
                    break;
                case M_SIZE_WORD:
                    size = -2;
                    break;
                case M_SIZE_LONG:
                    size = 4;
                    break;
            }
            if (size)
                outer = reader.get_fixed(size);
        }
    }

    else if (has_expr)
    {
        expr = reader.get_expr();
    }
    else if (has_data)
    {
        // get size of fixed data to read
        auto size = 0;
        switch (mode())
        {
            case MODE_IMMED_QUICK:
            case MODE_REG_QUICK:
            default:
                break; 

            // signed
            case MODE_ADDR_DISP:
            case MODE_PC_DISP:
            case MODE_MOVEP:
                size = -2;
                break;

            // unsigned
            case MODE_REGSET:
            case MODE_DIRECT_SHORT:
            case MODE_INDEX_BRIEF:
            case MODE_PC_INDEX_BRIEF:
                size = 2;
                break;

            // unsigned
            case MODE_DIRECT_LONG:
                size = 4;
                break;

            case MODE_REG:
            case MODE_PAIR:
            case MODE_BITFIELD:
                // save as expression
                break;

            case MODE_IMMED:
            case MODE_IMMED_LONG:
            case MODE_IMMED_SINGLE:
            case MODE_IMMED_XTND:
            case MODE_IMMED_PACKED:
            case MODE_IMMED_WORD:
            case MODE_IMMED_DOUBLE:
            case MODE_IMMED_BYTE:
                size = opc::m68k_size_immed[sz];
                if (size > 4)
                    size = 0;       // longer than LONG, save as expr
                else
                    size = -size;   // signed
                break;
        }
        expr = reader.get_fixed(size);
    }
}

}


#endif
