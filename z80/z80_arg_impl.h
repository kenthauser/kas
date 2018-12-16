#ifndef KAS_Z80_Z80_ARG_IMPL_H
#define KAS_Z80_Z80_ARG_IMPL_H


#include "z80_arg_defn.h"
#include "z80_error_messages.h"

namespace kas::z80
{

z80_arg_t::z80_arg_t(z80_arg_mode _mode, expr_t e)
    : mode(_mode), expr(e)
{
#if 0
    // big switch to decode arg
    // default: MODE_DIRECT or MODE_INDIRECT, with expr as value
    if (auto p = e.template get_p<z80_reg_t>())
    {
        reg = *p;
        if (_mode == MODE_INDIR)
            mode = MODE_REG_INDIR;
    }
    else if (auto p = e.template get_p<z80_regset_t>())
    {
        if (_mode != MODE_INDIR || p->kind() != -RC_OFFSET)
            mode = MODE_ERROR;
        else
        {
            reg = p->reg();
            if (auto e = p->expr_p())
                expr = *e;
            mode = MODE_REG_OFFSET;
        }
    }
#endif

}

template <typename...Ts>
const char * z80_arg_t::ok_for_target(Ts&&...args) const
{
    // 0. if parsed as error, propogate
    if (mode == MODE_ERROR)
        return err ? err : error_msg::ERR_argument;

    // OK
    return {};
}

void z80_arg_t::print(std::ostream& os) const
{
    switch (mode)
    {
        case MODE_DIRECT:
            os << expr;
            break;
        case MODE_INDIRECT:
            os << "(" << expr << ")";
            break;
        case MODE_REG:
            os << reg;
            break;
        case MODE_REG_INDIR:
            os << "(" << reg << ")";
            break;
        case MODE_REG_OFFSET:
            os << "(" << expr << ")";
            break;
        case MODE_ERROR:
            os << err;
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

    
}

#endif

