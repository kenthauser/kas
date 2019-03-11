#ifndef KAS_M68K_VALIDATE_H
#define KAS_M68K_VALIDATE_H

/******************************************************************************
 *
 * Instruction argument validation.
 *
 * There are four types of argument validation supported:
 *
 * 1) access mode validation: These modes are described in the
 *    M68K Programmers Reference Manual (eg: Table 2-4 in document
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

#include "m68k_expr_types.h"
#include "m68k_arg.h"
#include "m68k_error_messages.h"
#include "expr/expr_fits.h"
#include <array>


namespace kas::m68k::opc
{
using namespace meta;

// types used in validate
using expr_fits   = expression::expr_fits;
using fits_result = expression::fits_result;
using op_size_t   = core::opcode::op_size_t;

struct m68k_validate
{
    virtual fits_result ok  (m68k_arg_t& arg, m68k_size_t sz, expr_fits const& fits) const = 0;
    virtual fits_result size(m68k_arg_t& arg, m68k_size_t sz, expr_fits const& fits, op_size_t *) const
    { 
        // default: return "fits", don't update size
        return ok(arg, sz, fits);
    }
    
    // NB: literal types can't define dtors
    // virtual ~m68k_validate() = default;

    // set error coldfire & exceeds 3 words
    static fits_result coldfire_limit(fits_result result, op_size_t *size_p)
    {
#if 0
        // coldfire limits to 3 words
        if (cur_size_p && (arg_size.min + cur_size_p->min) > 6)
            if (!hw::cpu_defs[hw::limit_3w()]) 
                arg_size.set_error();     // XXX custom error message?
#endif    
        return result;
    }

};

// move to _impl
struct m68k_validate_args
{
    static constexpr auto MAX_ARGS = m68k_insn_t::MAX_ARGS;

    template <typename...Ts>
    constexpr m68k_validate_args(list<Ts...>)
        : arg_index { (Ts::value+1)... }
        , arg_count { sizeof...(Ts)    }
        {}

    static inline const m68k_validate *const *vals_base;
    static inline const char *const   *names_base;

    std::array<uint8_t, MAX_ARGS> arg_index;
    uint8_t                       arg_count;
};

}
#endif
