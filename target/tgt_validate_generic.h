#ifndef KAS_TARGET_TGT_VALIDATE_GENERIC_H
#define KAS_TARGET_TGT_VALIDATE_GENERIC_H

/******************************************************************************
 *
 * Instruction argument validation.
 *
 *****************************************************************************
 *
 * Two CRTP templated validators are also provided. These are used
 * to handle generic validations across most architectures
 *
 * tgt_val_reg  : used to validate a `register class` or single register
 *
 * tgt_val_range : used to validate expression within range of values
 *
 * These validators are templated. The CRTP pattern is used to add the 
 * ability to override support methods & extend functionality. 
 *
 *****************************************************************************/

#include "tgt_validate.h"

namespace kas::tgt::opc
{

// use CRTP to declare two common validator patterns
template <typename Derived, typename MCODE_T>
struct tgt_val_reg : MCODE_T::val_t
{
    using base_t      = tgt_val_reg;
    using derived_t   = Derived;

    using arg_t       = typename MCODE_T::arg_t;
    using arg_mode_t  = typename MCODE_T::arg_mode_t;
    using stmt_info_t = typename MCODE_T::stmt_info_t;
    using reg_t       = typename MCODE_T::reg_t;
    using reg_class_t = typename reg_t  ::reg_class_t;
    using reg_value_t = typename reg_t  ::reg_value_t;

    using mode_int_t  = std::underlying_type_t<arg_mode_t>;

    constexpr tgt_val_reg(reg_class_t r_class
                        , reg_value_t r_num     = ~0
                        , mode_int_t mode       = arg_mode_t::MODE_REG)
                   : r_class{r_class}, r_num(r_num), r_mode(mode) {}

    // CRTP cast
    auto constexpr& derived() const
        { return *static_cast<derived_t const*>(this); }
    
    // method to determine if `mode == REG`
    constexpr bool is_mode_ok(arg_t const& arg) const
    {
        return arg.mode() == r_mode;
    }

    // default value2reg: return `r_class`
    constexpr reg_class_t value2reg(unsigned& value) const
    {
        return r_class;
    }

    constexpr unsigned reg2value(reg_t const& reg) const
    {
        return reg.value(r_class);
    }

    // test if reg or reg_class
    constexpr bool is_single_register() const { return r_num != static_cast<reg_value_t>(~0); } 
   
    // test argument against validation
    fits_result ok(arg_t& arg, expr_fits const& fits) const override
    {
        if (!derived().is_mode_ok(arg))
            return fits.no;
        
        // here validating if mode of arg matches. validate if REG class matches desired
       if (arg.reg_p->kind(r_class) != r_class)
            return fits.no;

        // here reg-class matches. Test reg-num if specified
        // test if testing for rc_class only (ie. rc_value == ~0)
        if (!is_single_register())
            return fits.yes;

        // not default: look up actual rc_value
        if (arg.reg_p->value(r_class) == r_num)
            return fits.yes;

        return fits.no;
    }

    unsigned get_value(arg_t& arg) const override
    {
        return arg.reg_p->value(r_class);
    }
    
    void set_arg(arg_t& arg, unsigned value) const override
    {
        if (!is_single_register())
            arg.reg_p = &reg_t::find(r_class, value);
        else
            arg.reg_p = &reg_t::find(r_class, r_num);
    }

    reg_value_t r_num;
    reg_class_t r_class;
    mode_int_t  r_mode;
};
    
// the "range" validators default to zero size
template <typename MCODE_T>
struct tgt_val_range : MCODE_T::val_t
{
    using arg_t       = typename MCODE_T::arg_t;
    using arg_mode_t  = typename MCODE_T::arg_mode_t;
    using stmt_info_t = typename MCODE_T::stmt_info_t;

    constexpr tgt_val_range(int32_t min, int32_t max, int8_t zero = 0, int8_t size = 0)
                            : min(min), max(max), zero(zero), _size(size) {}

protected:
    fits_result range_ok(arg_t& arg, expr_fits const& fits) const
    {
        if (auto p = arg.get_fixed_p())
        {
            // if zero is mapped, block it.
            if (!*p && zero)
                return fits.no;
            return fits.fits(*p, min, max);
        }
        return fits.fits(arg.expr, min, max);

    }

public:
    fits_result ok(arg_t& arg, expr_fits const& fits) const override
    {
        // range is only for immediate args
        switch (arg.mode())
        {
            case arg_mode_t::MODE_IMMEDIATE:
            case arg_mode_t::MODE_IMMED_QUICK:
                return range_ok(arg, fits);
            default:
                return fits.no;
        }
    }

    fits_result size(arg_t& arg, MCODE_T const& mc, stmt_info_t const& info
                   , expr_fits const& fits, op_size_t& op_size) const override
    {
        if (_size) op_size += _size;
        return ok(arg, fits);
    }

    // get value to store in register
    unsigned get_value(arg_t& arg) const override
    {
        // if no size, it's QUICK
        arg.set_mode(_size ? arg_mode_t::MODE_IMMEDIATE : arg_mode_t::MODE_IMMED_QUICK);
        
        // calclulate value to insert in machine code
        auto p = arg.get_fixed_p();
        auto n = p ? *p : 0;
        return n == zero ? 0 : n;
    }

    void set_arg(arg_t& arg, unsigned value) const override
    {
        // calculate expression value from machine code
        arg.expr = value ? value : zero;
        arg.set_mode(_size ? arg_mode_t::MODE_IMMEDIATE : arg_mode_t::MODE_IMMED_QUICK);
    }
    
    int32_t  min, max;
    int8_t   zero, _size;
};


template <typename MCODE_T, typename T>
struct tgt_val_range_t : tgt_val_range<MCODE_T>
{
    tgt_val_range_t(uint8_t size = 0)
        : tgt_val_range<MCODE_T>(std::numeric_limits<T>::min()
                               , std::numeric_limits<T>::max()
                               , 0, size) {}
};

}
#endif
