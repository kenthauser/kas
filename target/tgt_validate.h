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
 * size      (arg, mcode, info, fits, size&) : updates `size`. return `fits_result`
 * get_value (arg)                    : get unsigned value for formatter insertion
 * set_arg   (arg, value)             : generate `arg` with formatter extracted `value`
 *                                    NB: `set_mode(mode)` executed after value extracted
 *
 * The `ok` validator checks basic `arg` charactistics.
 * The `size` validator presumes `ok` has passed. It also validates `info` values.
 * 
 * The theory is that a insn consists of base opcode (represented by `mcode`) and
 * parsed attributes from the actual opcode (such as condition code & and argument
 * type (width)). The `ok` validator is used to quickly eliminate clearly incompatible
 * arguments (such as the wrong register type, or an immediate value for register arg).
 * `ok` basically works on the `ARG_MODE`. 
 *
 * The `size` validator checks that all requirements on the arguement are met.
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
    using base_t      = tgt_validate;
    using mcode_t     = MCODE_T;
    using arg_t       = typename mcode_t::arg_t;
    using stmt_info_t = typename mcode_t::stmt_info_t;

    // need constexpr ctor for literal type
    constexpr tgt_validate() {}

    // if arg invalid for particular "size", error it out in `size` method
    virtual fits_result ok  (arg_t& arg, expr_fits const& fits) const = 0;
    virtual fits_result size(arg_t& arg, mcode_t const& mc, stmt_info_t const& info
                           , expr_fits const& fits, op_size_t&) const
    { 
        // default: return "fits", don't update size
        return ok(arg, fits);
    }

    // insert & extract values from opcode
    virtual unsigned get_value(arg_t& arg)           const { return {}; }
    virtual void     set_arg  (arg_t& arg, unsigned) const {}

    // tell emit that arg data has additional information
    virtual bool     has_data(arg_t& arg)            const { return true; }

    // NB: literal types can't define dtors
    //virtual ~tgt_validate() = default;
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

    // tgt_opc_branch just needs last validator (destination)
    auto last()  const { return iter(*this, arg_count-1); }

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


}
#endif
