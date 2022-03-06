#ifndef KAS_ARM_VALIDATE_REG_H
#define KAS_ARM_VALIDATE_REG_H

/******************************************************************************
 *
 * Instruction argument validation.
 *
 * See `target/tgt_validate.h` for information on virtual base class
 *
 *****************************************************************************/

#include "arm_validate.h"
#include "target/tgt_validate_generic.h"
#include "target/tgt_validate_branch.h"

namespace kas::arm::opc
{
using expr_fits   = expression::expr_fits;
using fits_result = expression::fits_result;
using op_size_t   = core::opcode::op_size_t;
using arg_mode_t  = typename arm_mcode_t::arg_mode_t;

//
// Derive common validators from generic templates
//

struct val_reg : tgt::opc::tgt_val_reg<val_reg, arm_mcode_t>
{
    using base_t::base_t;
};

struct val_range : tgt::opc::tgt_val_range<arm_mcode_t, int64_t>
{
    using base_t::base_t;
};

// unsigned range validator
struct val_range_u : tgt::opc::tgt_val_range_u<arm_mcode_t, int64_t>
{
    constexpr val_range_u(uint8_t n) : tgt_val_range_u(n) {}
};

template <unsigned SCALE>
struct val_range_scaled: tgt::opc::tgt_val_range<arm_mcode_t, int64_t, SCALE>
{
    using base_t = tgt::opc::tgt_val_range<arm_mcode_t, int64_t, SCALE>;
    using base_t::base_t;
};

struct val_arg_mode : tgt::opc::tgt_val_arg_mode<arm_mcode_t>
{
    using base_t::base_t;
};


struct val_prev  : tgt::opc::tgt_val_prev <arm_mcode_t>
{
    using base_t::base_t;
};
struct val_false : tgt::opc::tgt_val_false<arm_mcode_t> {};

// immediate with "IMMED_UPDATE" mode
struct val_range_update : val_range
{
    using val_range::val_range;
    
    fits_result ok(arg_t& arg, expr_fits const& fits) const override
    {
        // specialize "range" for IMMED_UPDATE args
        switch (arg.mode())
        {
            case arg_mode_t::MODE_IMMED_UPDATE:
                return range_ok(arg, fits);
            default:
                return fits.no;
        }
    }
};

// allow all RC_GEN, except PC (aka RC_GEN:15)
struct val_nopc : val_reg
{
    using base_t = val_reg;

    constexpr val_nopc() : base_t(RC_GEN) {}
    
    fits_result ok(arg_t& arg, expr_fits const& fits) const override
    {
        if (base_t::ok(arg, fits) == fits.yes)
            if (get_value(arg) != 15)
                return fits.yes;
        return fits.no;
    }
};


//
// ARM Specific Validators
//

struct val_regset : arm_mcode_t::val_t
{
    fits_result ok(arg_t& arg, expr_fits const& fits) const override
    {
        return arg.mode() == arg_mode_t::MODE_REGSET ? fits.yes : fits.no;
    }
    
    unsigned get_value(arg_t& arg) const override
    {
        switch (arg.mode())
        {
            case arg_mode_t::MODE_IMMEDIATE:
                arg.set_mode(arg_mode_t::MODE_IMMED_QUICK);
                // FALLSTHRU
            case arg_mode_t::MODE_IMMED_QUICK:
                if (auto p = arg.expr.get_fixed_p())
                    return *p;
                return 0;
            case arg_mode_t::MODE_REGSET:
                return arg.regset_p->value();
            default:
            // calclulate value to insert in machine code
                return 0;
        }
    }

    void set_arg(arg_t& arg, unsigned value) const override
    {
        // calculate expression value from machine code
        arg.expr = value;
        arg.set_mode(arg_mode_t::MODE_IMMED_QUICK);
    }
    
};

struct val_regset_single : val_regset
{
    using base_t = val_regset;

    fits_result ok(arg_t& arg, expr_fits const& fits) const override
    {
        if (base_t::ok(arg, fits) == fits.yes)
            if (arg.regset_p->is_single())
                return fits.yes;
        return fits.no;
    }
};

struct val_regset_user : val_regset
{
    fits_result ok(arg_t& arg, expr_fits const& fits) const override
    {
        return arg.mode() == arg_mode_t::MODE_REGSET_USER ? fits.yes : fits.no;
    }
};
}
#endif
