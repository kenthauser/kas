#ifndef KAS_ARM_ARM_ARG_IMPL_H
#define KAS_ARM_ARM_ARG_IMPL_H

#include "arm_arg.h"
#include "arm_error_messages.h"
#include "kas_core/core_emit.h"

namespace kas::arm
{

// support type constructors
arm_arg_t::arm_arg_t(arm_shift_arg const& arg)
    : shift(arg.shift), base_t(arg.mode, arg.expr) {}

arm_arg_t::arm_arg_t(arm_indirect const& indir)
    : indir (indir)
    , base_t(MODE_REG_INDIR)
{
    expr = indir.offset;
    reg  = indir.base_reg;
}

// support routine to encode indirect arg terms
template <typename Context>
void arm_indirect::operator()(Context const& ctx)
{
    auto& args = x3::_attr(ctx);
    x3::_val(ctx) = *this;
}

// support routine to encode parsed shift arg
template <typename Context>
void arm_shift_arg::operator()(Context const& ctx)
{
    auto make_error = [this](const char *)
        {
            mode = MODE_ERROR;
        };


    // arg list: expr or reg
    auto& arg = x3::_attr(ctx);

    // test if arg is `reg_t`
    constexpr auto is_reg_arg = std::is_same_v<
                                        arm_reg_t
                                      , std::remove_reference_t<decltype(arg)>
                                      >;

    if (shift_op >= NUM_ARM_SHIFT)
        throw std::logic_error("ARM_SHIFT_* out of range");

    // RRX is special
    if (shift_op == ARM_SHIFT_RRX)
    {
        // type == 3, otherwise zeros
        shift.type = 3;
    }
    else 
    {
        // test if `immed` or `reg` format
        auto op = shift_op - 1;
        shift.is_reg = op & 1;
        shift.type   = op >> 1;
        if (shift.is_reg)
        {
            if constexpr (!is_reg_arg)
                make_error("register required");
            else
            {
                // validate 
                if (arg.kind(RC_GEN) != RC_GEN)
                    make_error("general register required");
                shift.reg = arg.value(RC_GEN);
            }

        }
        else
        {
            // here for "constant" shift
            if constexpr (is_reg_arg)
                throw std::logic_error("expected expr not register");
            else
            {
                auto p = arg.get_fixed_p();
                if (!p)
                    make_error("shift must be constant value");
                else
                {
                    // allow ASR & LSR shifts of 32 (map to zero)
                    unsigned value = *p;
                    if (shift.type == 1 || shift.type == 2)
                        if (value == 32)
                           value = 0;

                    // translate "no shift" as LSR #0 (ie NONE)
                    if (value == 0)
                        shift.type = 0;

                    if (value < 32)
                        shift.ext = value;
                    else
                        make_error("shift out of range");
                }
            }
        }
    }
    x3::_val(ctx) = *this;
}

}

#endif

