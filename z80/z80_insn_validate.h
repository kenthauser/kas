#ifndef KAS_Z80_VALIDATE_H
#define KAS_Z80_VALIDATE_H

/******************************************************************************
 *
 * Instruction argument validation.
 *
 * There are four types of argument validation supported:
 *
 * 1) access mode validation: These modes are described in the
 *    Z80 Programmers Reference Manual (eg: Table 2-4 in document
 *    M680000PM/AD) and used throughout opcode descriptions.
 *
 * 2) register class: based on class enum (eg: RC_DATA or RC_CTRL)
 *
 * 3) single register: base on class enum & value
 *    (eg: RC_CTRL:0x800 for `USP`)
 *
 * 4) arbitrary function: these can be used to, for example, validate
 *    the many constant ranges allowed by various instructions. Examples
 *    include `MOVEQ` (signed 8 bits), `ADDQ` (1-8 inclusive), etc.
 *    Function may also specify argument size calculation functions.
 *
 * For each type of validation, support three methods:
 *      - const char* name()                 : return name of validation
 *      - fits_result ok(arg&, info&, fits&) : test argument against validation
 *      - op_size_t   size(arg&, info&, fits&, size_p*) : bytes required by arg
 *
 *
 *****************************************************************************
 *
 * Implementation
 *
 * 
 *
 *
 *

 *****************************************************************************/

#include "z80_types.h"
#include "z80_arg_defn.h"
#include "z80_error_messages.h"
#include "expr/expr_fits.h"
#include <array>


namespace kas::z80::opc
{
using namespace meta;

// types used in validate
using expr_fits   = expression::expr_fits;
using fits_result = expression::fits_result;
using op_size_t   = core::opcode::op_size_t;

struct z80_validate
{
    virtual fits_result ok  (z80_arg_t& arg, expr_fits const& fits) const = 0;
    virtual fits_result size(z80_arg_t& arg, expr_fits const& fits, op_size_t&) const
    { 
        // default: return "fits", don't update size
        return ok(arg, fits);
    }

    // insert & extract values from opcode
    virtual unsigned get_value(z80_arg_t& arg)           const { return {}; }
    virtual void     set_arg  (z80_arg_t& arg, unsigned) const {}

    // NB: literal types can't define dtors
    // virtual ~z80_validate() = default;
};

// move to _impl
struct z80_validate_args
{
    static constexpr auto MAX_ARGS = z80_insn_t::MAX_ARGS;
    using val_index_t = uint8_t;

    template <typename...Ts>
    constexpr z80_validate_args(list<Ts...>)
        : arg_index { (Ts::value+1)... }
        , arg_count { sizeof...(Ts)    }
        {}

    struct iter : std::iterator<std::forward_iterator_tag, z80_validate>
    {
        iter(z80_validate_args const& obj, val_index_t index = 0) : obj(obj), index(index) {}

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
        val_index_t index;
        z80_validate_args const& obj;
    };

    
public:
    auto size()  const { return arg_count; }
    auto begin() const { return iter(*this);            }
    auto end()   const { return iter(*this, arg_count); }

    //static void set_base (z80_validate * const *base)  { vals_base = base;   }
    //static void set_names(const char * const *names)   { names_base = names; }

//private:
    static inline const z80_validate *const *vals_base;
    static inline const char *const   *names_base;

    std::array<val_index_t, MAX_ARGS> arg_index;
    val_index_t                       arg_count;

};

}
#endif
