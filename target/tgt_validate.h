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
 *****************************************************************************/

#include "expr/expr_fits.h"
#include <array>


namespace kas::tgt::opc
{
using namespace meta;

// types used in validate
using expr_fits   = expression::expr_fits;
using fits_result = expression::fits_result;
using op_size_t   = core::opcode::op_size_t;

template <typename MCODE_T>
struct tgt_validate
{
    using arg_t      = typename MCODE_T::arg_t;

    // if arg invalid for particular "size", error it out in `size` method
    virtual fits_result ok  (arg_t& arg, expr_fits const& fits) const = 0;
    virtual fits_result size(arg_t& arg, uint8_t sz, expr_fits const& fits, op_size_t&) const
    { 
        // default: return "fits", don't update size
        return ok(arg, fits);
    }

    // insert & extract values from opcode
    virtual unsigned get_value(arg_t& arg)           const { return {}; }
    virtual void     set_arg  (arg_t& arg, unsigned) const {}

    // `not_saved` only called on args that are `OK`
    // no validator returning `not_saved::true` are in "*LIST*" serialize validators
    virtual bool     all_saved(arg_t& arg) const
    { 
        expr_fits fits;
        return fits.zero(arg.expr) == fits.yes;
    }

    // NB: literal types can't define dtors
    // virtual ~tgt_validate() = default;
};

// move to _impl
template <typename MCODE_T>
struct tgt_validate_args
{
    static constexpr auto MAX_ARGS = MCODE_T::MAX_ARGS;
    using val_idx_t = typename MCODE_T::val_idx_t;

    template <typename...Ts>
    constexpr tgt_validate_args(list<Ts...>)
        : arg_index { (Ts::value+1)... }
        , arg_count { sizeof...(Ts)    }
        {}

    struct iter : std::iterator<std::forward_iterator_tag, tgt_validate<MCODE_T>>
    {
        iter(tgt_validate_args const& obj, val_idx_t index = 0) : obj(obj), index(index) {}

    private:
        // get index of current validator
        auto val_index() const
        {
            return obj.arg_index[index] - 1;
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
            return &obj != &other.obj || index != other.index;
        }

    private:
        val_idx_t index;
        tgt_validate_args const& obj;
    };

    
public:
    auto size()  const { return arg_count; }
    auto begin() const { return iter(*this);            }
    auto end()   const { return iter(*this, arg_count); }

    void print(std::ostream&) const;

//private:
    friend std::ostream& operator<<(std::ostream& os, tgt_validate_args const& vals)
    {
        vals.print(os);
        return os;
    }

    // validate is abstract class. access via pointers to instances
    static inline const tgt_validate<MCODE_T> *const *vals_base;
    static inline const char *                 const *names_base;

    std::array<val_idx_t, MAX_ARGS> arg_index;
    val_idx_t                       arg_count;

};


template <typename MCODE_T>
void tgt_validate_args<MCODE_T>::print(std::ostream& os) const
{
    os << MCODE_T::BASE_NAME::value << "::validate_args:";
    os << " count = " << +arg_count << " indexs = [";
    char delim{};
    for (auto n = 0; n < arg_count; ++n)
    {
        if (delim) os << delim;
        os << +arg_index[n];
        delim = ',';
    }
    os << "] vals = [";
    delim = {};
    for (auto n = 0; n < arg_count; ++n)
    {
        if (delim) os << delim;
        os << names_base[arg_index[n]-1];
        delim = ',';
    }
    os << "]" << std::endl;
}

}
#endif
