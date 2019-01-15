#ifndef KAS_Z80_Z80_ARG_IMPL_H
#define KAS_Z80_Z80_ARG_IMPL_H


#include "z80_arg.h"
#include "z80_error_messages.h"
#include "kas_core/core_emit.h"

namespace kas::z80
{

z80_arg_t::z80_arg_t(std::pair<expr_t, int> const& parsed) : base_t(parsed.second, parsed.first)
{
    auto set_prefix = [&](int base_mode)
    {
        prefix = reg.value();
        switch (prefix)
        {
            case 0xdd:
                break;
            case 0xfd:
                ++base_mode;
                break;
            default:
                base_mode = MODE_ERROR;
                err = error_msg::ERR_argument;
                break;
        }
        _mode = base_mode;
    };
    
    // big switch to decode arg
    // default: MODE_DIRECT or MODE_INDIRECT, with expr as value
    if (auto p = expr.template get_p<z80_reg_t>())
    {
        reg = *p;
        expr = {};

        switch (_mode)
        {
            case MODE_DIRECT:
                if (reg.kind(RC_IDX) == RC_IDX)
                    set_prefix(MODE_REG_IX);
                else
                    _mode = MODE_REG;
                break;

            case MODE_INDIRECT:
                if (reg.kind(RC_IDX) == RC_IDX)
                    set_prefix(MODE_REG_INDIR_IX);
                else
                    _mode = MODE_REG_INDIR;
                break;

            case MODE_IMMEDIATE:
            default:
                _mode = MODE_ERROR;
                err   = error_msg::ERR_argument;
                break;

        }
    }

    else if (auto p = expr.template get_p<z80_reg_set>())
    {
        if (_mode != MODE_INDIRECT || p->kind() != -z80_reg_set::RS_OFFSET)
        {
            _mode = MODE_ERROR;
            err   = error_msg::ERR_argument;
        }
        else
        {
            reg  = p->reg();
            expr = p->offset();
            set_prefix(MODE_REG_OFFSET_IX);    
        }
    }

    //std::cout << "z80_arg_t ctor: expr = " << expr << " mode = " << _mode << " pfx = " << +prefix << std::endl;
}

void z80_arg_t::set_mode(uint8_t mode)
{
    _mode = mode;
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

#if 1
template <typename...Ts>
const char * z80_arg_t::ok_for_target(Ts&&...args) const
{
    // 0. if parsed as error, propogate
    if (mode() == MODE_ERROR)
        return err ? err : error_msg::ERR_argument;

    // OK
    return {};
}
#endif

void z80_arg_t::print(std::ostream& os) const
{
    switch (mode())
    {
        case MODE_DIRECT:
            os << expr;
            break;
        case MODE_INDIRECT:
            os << "(" << expr << ")";
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
            os << "(" << reg << "+" << expr << ")";
            break;
        case MODE_ERROR:
            if (err)
                os << err;
            else
                os << "Err: *UNDEFINED*";
            break;
        default:
            os << "** INVALID **";
            break;
    }
}

auto z80_arg_t::size(expression::expr_fits const& fits) -> op_size_t
{
    return 0;
}

template <typename Inserter>
bool z80_arg_t::serialize(Inserter& inserter, bool& val_ok)
{
    auto save_expr = [&](auto size) -> bool
        {
            // suppress writes of zero
            auto p = expr.get_fixed_p();
            if (p && !*p)
            {
                val_ok = true;      // validator saved all data.
                return false;       // and no expression.
            }
            val_ok = false;
            return !inserter(std::move(expr), size);
        };
    
    switch(mode())
    {
        case MODE_REG_INDIR_IX:
        case MODE_REG_INDIR_IY:
        default:
            // nothing to restore. Mode says it all.
            val_ok = true;
            break;

        case MODE_REG:
        case MODE_REG_INDIR:
            // if saved as GEN register
            if (val_ok)
                break;
            {
                auto r_class = reg.kind();
                auto value   = reg.value();
                inserter((r_class << 8) | value, 2);
                break;
            }

        case MODE_DIRECT:
        case MODE_INDIRECT:
        case MODE_REG_OFFSET_IX:
        case MODE_REG_OFFSET_IY:
            if (val_ok)
                break;
            return save_expr(2);
    }

    return false;   // no expression
}

template <typename Reader>
void z80_arg_t::extract(Reader& reader, bool has_data, bool has_expr)
{
    if (has_expr)
    {
        expr = reader.get_expr();
        if (auto p = expr.template get_p<z80_reg_t>())
            reg = *p;
    }
    else if (has_data)
    {
        auto value = reader.get_fixed(2);
        switch (mode())
        {
            default:
                expr = value;
                break;
            case MODE_REG:
            case MODE_REG_INDIR:
                reg = z80_reg_t(value >> 8, value & 0xff);
                break;
        }
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

