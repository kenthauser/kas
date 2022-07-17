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

    // value to allow all registers (ie: only test r_class)
    static constexpr auto all_regs = -1;

    constexpr tgt_val_reg(reg_class_t r_class
                        , int         r_num = all_regs
                        , mode_int_t  mode  = arg_mode_t::MODE_REG)
                   : r_class{r_class}, r_num(r_num), r_mode{mode} {}

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
    constexpr bool is_single_register() const
                { return r_num != static_cast<reg_value_t>(all_regs); } 
   
    // test argument against validation
    fits_result ok(arg_t& arg, expr_fits const& fits) const override
    {
        if (!derived().is_mode_ok(arg))
            return fits.no;
        
        // here validating if mode of arg matches.
        // validate if REG class matches desired
        if (arg.reg_p->kind(r_class) != r_class)
            return fits.no;

        // here reg-class matches. Test reg-num if specified
        // test if testing for rc_class only (ie. rc_value == ~0)
        if (!is_single_register())
            return fits.yes;

        // not default: look up actual rc_value
        if (get_value(arg) == r_num)
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

    // "register" format has no other data to save
    bool has_data(arg_t&) const override { return false; }

    reg_value_t r_num;
    reg_class_t r_class;
    mode_int_t  r_mode;
};

// validate current is *same* as previous `REG` argument
template <typename MCODE_T>
struct tgt_val_prev : MCODE_T::val_t
{
    using base_t      = tgt_val_prev;
    using arg_t       = typename MCODE_T::arg_t;
    using arg_mode_t  = typename MCODE_T::arg_mode_t;
    using stmt_info_t = typename MCODE_T::stmt_info_t;
    using reg_t       = typename MCODE_T::reg_t;
    using reg_class_t = typename reg_t  ::reg_class_t;
    using reg_value_t = typename reg_t  ::reg_value_t;

    using mode_int_t  = std::underlying_type_t<arg_mode_t>;

    constexpr tgt_val_prev(uint8_t n) : prev(n) {}
    
    fits_result ok(arg_t& arg, expr_fits const& fits) const override
    {
        if (arg.mode() != arg_mode_t::MODE_REG)
            return fits.no;

        // NB: need to generalize if other than array is used for args
        auto& p = (&arg)[-prev];
        
        if (p.reg_p == arg.reg_p)
            return fits.yes;
        return fits.no;
    }
    
    fits_result size(arg_t& arg, uint8_t sz
                   , expr_fits const& fits, op_size_t& op_size) const override
    {
        return fits.yes;
    }

    void set_arg(arg_t& arg, unsigned value) const override
    {
        arg = (&arg)[-prev];
    }
    
    uint8_t prev;
};
    
// the "range" validators default to zero size
template <typename MCODE_T, typename RANGE_VALUE_T = int16_t, unsigned SCALE = 0>
struct tgt_val_range : MCODE_T::val_t
{
    using base_t      = tgt_val_range;
    using value_t     = RANGE_VALUE_T;
    using arg_t       = typename MCODE_T::arg_t;
    using arg_mode_t  = typename MCODE_T::arg_mode_t;
    using stmt_info_t = typename MCODE_T::stmt_info_t;

    // make SCALE == 1 -> SCALE = 0
    static constexpr auto scale = (SCALE != 1) ? SCALE : 0;
    static constexpr auto mask  = (1 << scale) - 1;

    constexpr tgt_val_range(value_t min
                          , value_t max
                          , int8_t  zero = 0
                          , int8_t  size = 0)
               : min(min), max(max), zero(zero), _size(size) {}

protected:

    fits_result range_ok(arg_t& arg, expr_fits const& fits) const
    {
        if (auto p = arg.get_fixed_p())
        {
            auto n = *p;
            //std::cout << "tgt_val_range::range_ok: n = " << +n << ", scale = " << +scale << std::endl;

            // if zero is mapped, block it.
            if (!n && zero)
                return fits.no;
            
            // if scaled, be sure LSBs are zero
            if constexpr (mask != 0)
                if (mask & n)
                    return fits.no;

            // scale is no-op if not scaled

            return fits.fits(n >> scale, min, max);
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

    fits_result size(arg_t& arg, uint8_t sz
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
        // if constant, store in opcode. substitue value for zero as required
        auto p = arg.expr.get_fixed_p();
        auto n = p ? *p : 0;
        //std::cout << "tgt_val_range: n = " << +n << ", scale = " << +scale << std::endl;
        return n == zero ? 0 : (n >> scale);
    }

    void set_arg(arg_t& arg, unsigned value) const override
    {
        // calculate expression value from machine code
        arg.expr = value ? (value << scale) : zero;
        arg.set_mode(_size ? arg_mode_t::MODE_IMMEDIATE : arg_mode_t::MODE_IMMED_QUICK);
    }
    
    value_t  min, max;
    int8_t   zero, _size;
};


// validate unsigned range of `N` bits
template <typename MCODE_T, typename RANGE_VALUE_T = int16_t, unsigned SCALE = 0>
struct tgt_val_range_u : tgt_val_range<MCODE_T, RANGE_VALUE_T, SCALE>
{
    using base_t      = tgt_val_range<MCODE_T, RANGE_VALUE_T, SCALE>;

    constexpr tgt_val_range_u(uint8_t n)
            : base_t (0, (1L << n) - 1) {}
};


template <typename MCODE_T, typename T, typename base_t = tgt_val_range<MCODE_T>>
struct tgt_val_range_t : tgt_val_range<MCODE_T>
{
    using U = std::make_unsigned_t<T>;
    tgt_val_range_t(uint8_t size = sizeof(T))
        : tgt_val_range<MCODE_T>(std::numeric_limits<T>::min()
                               , std::numeric_limits<U>::max()
                               , 0, size) {}
};

// generic to validate only MODE
// derive from `tgt_val_range` & override `OK`
template <typename MCODE_T>
struct tgt_val_arg_mode : tgt_val_range<MCODE_T>
{
    using base_t      = tgt_val_arg_mode;
    using mcode_t     = MCODE_T;
    using arg_t       = typename mcode_t::arg_t;
    using arg_mode_t  = typename mcode_t::arg_mode_t;

    constexpr tgt_val_arg_mode(int mode) 
                : mode{static_cast<arg_mode_t>(mode)}
                , tgt_val_range<MCODE_T>(0, 0) {}

    fits_result ok(arg_t& arg, expr_fits const& fits) const override
    {
        if (arg.mode() == mode)
            return fits.yes;
        return fits.no;
    }
    
    arg_mode_t mode;
};


// special for development: placeholder validator that is always false
template <typename MCODE_T>
struct tgt_val_false : MCODE_T::val_t
{
    using arg_t = typename MCODE_T::arg_t;
    fits_result ok(arg_t& arg, expr_fits const& fits) const override
    {
        return fits.no;
    }
};
}
#endif
