#ifndef KAS_TARGET_TGT_VALIDATE_H
#define KAS_TARGET_TGT_VALIDATE_H

/******************************************************************************
 *
 * Instruction argument validation.
 *
 * Two types are defined. Both are templated with MCODE_T
 *
 * `tgt_validate` is a virtual base class which tests arguments.
 *
 * Two methods: `ok` & `size`, both return `expression::fits_result`
 * `ok`   tests if arg passes validation test.
 * `size` performs `ok` & then evaluates size of arg.
 *
 *
 * The second type is `tgt_validate_args`
 *
 * This type is constructed with a `meta::list` of validators.
 * This type can create iterator over validators. This iterator
 * dereferences to the iterator & supports a `name` method which
 * returns printable name.
 *
 *****************************************************************************
 *
 * `tgt_validate` description
 *
 * Each validator has 4 virtual methods. 
 *
 * ok        (arg, fits)              : returns `fits_result`
 * size      (arg, info, fits, size&) : updates `size` if `ok`. returns `ok`
 * get_value (arg)                    : get unsigned value for formatter insertion
 * set_arg   (arg, value)             : generate `arg` with formatter extracted `value`
 *                                    NB: `set_mode(mode)` executed after value extracted
 *
 * NB: `ok` validator presumed to store register values
 *
 *
 *****************************************************************************
 *
 * `tgt_validate_args` description
 *
 * Each `tgt_validate_args` instance is used to validate the entire
 * set of arguments for an instruction.
 *
 * There are 3 methods:
 *
 * begin()      : iterator to first validator
 * end()        : const iterator to last validator
 * size()       : returns number of args in list
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

#include "expr/expr_fits.h"
#include <array>


namespace kas::tgt::opc
{

// types used in validate
using expr_fits   = expression::expr_fits;
using fits_result = expression::fits_result;
using op_size_t   = core::opcode::op_size_t;

template <typename MCODE_T>
struct tgt_validate
{
    using arg_t       = typename MCODE_T::arg_t;
    using stmt_info_t = typename MCODE_T::stmt_info_t;

    // if arg invalid for particular "size", error it out in `size` method
    virtual fits_result ok  (arg_t& arg, stmt_info_t const& info, expr_fits const& fits) const = 0;
    virtual fits_result size(arg_t& arg, stmt_info_t const& info, expr_fits const& fits, op_size_t&) const
    { 
        // default: return "fits", don't update size
        return ok(arg, info, fits);
    }

    // insert & extract values from opcode
    virtual unsigned get_value(arg_t& arg)           const { return {}; }
    virtual void     set_arg  (arg_t& arg, unsigned) const {}
    
    // NB: literal types can't define dtors
    // virtual ~tgt_validate() = default;
};


template <typename MCODE_T>
struct tgt_validate_args
{
    static constexpr auto MAX_ARGS = MCODE_T::MAX_ARGS;
    using val_idx_t = typename MCODE_T::val_idx_t;
    using val_t     = typename MCODE_T::val_t;
    using adder_t   = typename MCODE_T::adder_t;

    
    // XXX 2019/06/01 KHB: std::array<> initializer should catch MAX_ARGs exceeded
    // XXX apple clang doesn't
    template <typename...Ts>
    constexpr tgt_validate_args(list<Ts...>)
        : arg_index { Ts::value...  }
        , arg_count { sizeof...(Ts) }
        {}

    struct iter : std::iterator<std::forward_iterator_tag, tgt_validate<MCODE_T>>
    {
        iter(tgt_validate_args const& obj, val_idx_t index = 0) : obj_p(&obj), index(index) {}

    private:
        // get index of current validator
        auto val_index() const
        {
            return obj_p->arg_index[index];
        }

    public:
        auto name() const
        {
            return names_base[val_index()];
        }
        
        auto operator->() const
        {
            return vals_base[val_index()];
        }

        auto& operator*() const
        {
            return *operator->();
        }

        auto& operator++()
        {
            ++index;
            return *this;
        }

        auto operator++(int)
        {
            auto value = *this;
            ++index;
            return value;
        }
        
        auto operator!=(iter const& other) const
        {
            return obj_p != other.obj_p || index != other.index;
        }

    private:
        val_idx_t index;
        tgt_validate_args const *obj_p;
    };

    
public:
    auto size()  const { return arg_count; }
    auto begin() const { return iter(*this);            }
    auto end()   const { return iter(*this, arg_count); }

    void print(std::ostream&) const;

private:
    friend std::ostream& operator<<(std::ostream& os, tgt_validate_args const& vals)
    {
        vals.print(os);
        return os;
    }

    // validate is abstract class. access via pointers to instances
    // initialized by `adder_t`
    friend adder_t;

    static inline const val_t *const *vals_base;
    static inline const char  *const *names_base;

    std::array<val_idx_t, MAX_ARGS> arg_index;
    val_idx_t                       arg_count;
};


// Declare common validators as CRTP. Override support methods as required

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
    fits_result ok(arg_t& arg, stmt_info_t const& info, expr_fits const& fits) const override
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
        auto reg_class = derived().value2reg(value);
        
        if (!is_single_register())
            arg.reg_p = &reg_t::get(reg_class, value);
        else
            arg.reg_p = &reg_t::get(reg_class, r_num);
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

    fits_result ok(arg_t& arg, stmt_info_t const& info, expr_fits const& fits) const override
    {
        // range is only for immediate args
        switch (arg.mode())
        {
            case arg_mode_t::MODE_IMMEDIATE:
            case arg_mode_t::MODE_IMMED_QUICK:
                if (auto p = arg.expr().get_fixed_p())
                {
                    // if zero is mapped, block it.
                    if (!*p && zero)
                        return fits.no;
                    return fits.fits(*p, min, max);
                }
                return fits.fits(arg.expr(), min, max);
            default:
                return fits.no;
        }
    }

    fits_result size(arg_t& arg, stmt_info_t const& info, expr_fits const& fits
                                                    , op_size_t& op_size) const override
    {
        if (_size) op_size += _size;
        return ok(arg, info, fits);
    }

    // get value to store in register
    unsigned get_value(arg_t& arg) const override
    {
        // if no size, it's QUICK
        arg.set_mode(_size ? arg_mode_t::MODE_IMMEDIATE : arg_mode_t::MODE_IMMED_QUICK);
        
        // calclulate value to insert in machine code
        auto p = arg.expr().get_fixed_p();
        auto n = p ? *p : 0;
        return n == zero ? 0 : n;
    }

    void set_arg(arg_t& arg, unsigned value) const override
    {
        // calculate expression value from machine code
        //arg.expr = value ? value : zero;
        arg.set(value);     // XXX token change
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
