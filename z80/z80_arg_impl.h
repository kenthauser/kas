#ifndef KAS_Z80_Z80_ARG_IMPL_H
#define KAS_Z80_Z80_ARG_IMPL_H


#include "z80_arg.h"
#include "z80_error_messages.h"
#include "kas_core/core_emit.h"

namespace kas::z80
{

z80_arg_t::z80_arg_t(std::pair<expr_t, z80_arg_mode> const& parsed) : base_t(parsed.second, parsed.first)
{
    const char *msg {};

    auto set_prefix = [&](z80_arg_mode base_mode)
    {
        prefix = reg.value();
        switch (prefix)
        {
            case 0xdd:
                break;
            case 0xfd:
                // IY prefix one above IX
                base_mode = static_cast<z80_arg_mode>(base_mode + 1);
                break;
            default:
                msg = error_msg::ERR_argument;
                break;
        }
        set_mode(base_mode);
    };
    
    // big switch to decode arg
    // default: MODE_DIRECT or MODE_INDIRECT, with expr as value
    if (auto p = expr.template get_p<z80_reg_t>())
    {
        reg = *p;
        expr = {};

        switch (mode())
        {
            case MODE_DIRECT:
                if (reg.kind(RC_IDX) == RC_IDX)
                    set_prefix(MODE_REG_IX);
                else
                    set_mode(MODE_REG);
                break;

            case MODE_INDIRECT:
                if (reg.kind(RC_IDX) == RC_IDX)
                    set_prefix(MODE_REG_INDIR_IX);
                else
                    set_mode(MODE_REG_INDIR);
                break;

            case MODE_IMMEDIATE:
            default:
                msg = error_msg::ERR_argument;
                break;

        }
    }

    // regset interface is obscure, needs work
    else if (auto p = expr.template get_p<z80_reg_set>())
    {
        if (mode() != MODE_INDIRECT || p->kind() != -z80_reg_set::RS_OFFSET)
        {
            msg  = error_msg::ERR_argument;
        }
        else
        {
            reg  = p->reg();
            expr = p->offset();
            set_prefix(MODE_REG_OFFSET_IX);    
        }
    }

    if (msg)
    {
        err = kas::parser::kas_diag::error(msg, *this).ref();
        set_mode(MODE_ERROR);
    }
    //std::cout << "z80_arg_t ctor: expr = " << expr << " mode = " << _mode << " pfx = " << +prefix << std::endl;
}

// constant if register or constant expression
bool z80_arg_t::is_const() const
{
    switch (mode())
    {
        case MODE_REG:
        case MODE_REG_IX:
        case MODE_REG_IY:
        case MODE_REG_INDIR:
        case MODE_REG_INDIR_IX:
        case MODE_REG_INDIR_IY:
            return true;
        default:
            break;
    }
    return expr.get_fixed_p();
}

void z80_arg_t::set_mode(unsigned mode)
{
    base_t::set_mode(mode);
    switch (mode)
    {
        case MODE_REG_IX:
        case MODE_REG_INDIR_IX:
        case MODE_REG_OFFSET_IX:
            prefix = 0xdd;
            break;
        case MODE_REG_IY:
        case MODE_REG_INDIR_IY:
        case MODE_REG_OFFSET_IY:
            prefix = 0xfd;
            break;
        default:
            break;
    }
}

void z80_arg_t::set_expr(expr_t& e)
{
    if (auto p = e.template get_p<z80_reg_t>())
        reg = *p;
    else
        expr = e;
}

#if 0
template <typename...Ts>
const char * z80_arg_t::ok_for_target(Ts&&...args) const
{
    // OK
    return {};
}
#endif

void z80_arg_t::print(std::ostream& os) const
{
    switch (mode())
    {
        case MODE_DIRECT:
        case MODE_IMMED_QUICK:
            os << expr;
            break;
        case MODE_INDIRECT:
            os << "(" << expr << ")";
            break;
        case MODE_IMMEDIATE:
            os << "#" << expr;
            break;
        case MODE_REG:
        case MODE_REG_IX:
        case MODE_REG_IY:
            os << reg;
            break;
        case MODE_REG_INDIR:
        case MODE_REG_INDIR_IX:
        case MODE_REG_INDIR_IY:
            os << "(" << reg << ")";
            break;
        case MODE_REG_OFFSET_IX:
        case MODE_REG_OFFSET_IY:
            // print (ix-N) iff (N == fixed && N < 0)
            // else print (ix+N)
            if (auto p = expr.get_fixed_p())
                if (*p < 0)
                {
                    os << "(" << reg << std::dec << *p << ")";
                    break;
                }
            os << "(" << reg << "+" << expr << ")";
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
            os << "** INVALID: " << +mode() << " **";
            break;
    }
}

auto z80_arg_t::size(expression::expr_fits const& fits) -> op_size_t
{
    return 0;
}

// optimally save all z80 arguments
template <typename Inserter>
bool z80_arg_t::serialize(Inserter& inserter, uint8_t sz, bool& completely_saved)
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
    
    switch(mode())
    {
        case MODE_IMMED_QUICK:
        default:
            // nothing to save. Mode says it all.
            completely_saved = true;
            break;

        case MODE_REG:
        case MODE_REG_INDIR:
            // if saved as GEN register
            if (!completely_saved)
            {
                auto r_class = reg.kind();
                auto value   = reg.value();
                inserter((r_class << 8) | value, 2);
            }
            break; 

        case MODE_DIRECT:
        case MODE_INDIRECT:
        case MODE_IMMEDIATE:
            return save_expr(2);

        case MODE_REG_OFFSET_IX:
        case MODE_REG_OFFSET_IY:
            return save_expr(1);
    }

    return false;   // no expression
}

// handle all cases serialized above
template <typename Reader>
void z80_arg_t::extract(Reader& reader, uint8_t sz, bool has_data, bool has_expr)
{
    if (has_expr)
    {
        expr = reader.get_expr();
        if (auto p = expr.template get_p<z80_reg_t>())
            reg = *p;
    }
    else if (has_data)
    {
        auto size = 2;  // bytes to read
        switch (mode())
        {
            default:
                break;
            case MODE_IMMEDIATE:
                size = -2;      // signed word
                break;
            case MODE_REG_OFFSET_IX:
            case MODE_REG_OFFSET_IY:
                size = -1;      // signed byte
                break;
            case MODE_REG:
            case MODE_REG_INDIR:
            {
                auto value = reader.get_fixed(2);
                reg = z80_reg_t(value >> 8, value & 0xff);
                return;         // done
            }
        }

        expr = reader.get_fixed(size);
    }
}

// size is from validator
void z80_arg_t::emit(core::emit_base& base, unsigned size) const
{
    // IX/IY offsets are emitted by base code. don't double emit
    switch (mode())
    {
        case MODE_REG_INDIR_IX:
        case MODE_REG_INDIR_IY:
        case MODE_REG_OFFSET_IX:
        case MODE_REG_OFFSET_IY:
            break;

        default:
            base << core::set_size(size) << expr; 
            break;
    }
}
    
}

#endif

