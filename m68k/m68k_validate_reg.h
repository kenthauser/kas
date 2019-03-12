#ifndef KAS_M68K_VALIDATE_REG_H
#define KAS_M68K_VALIDATE_REG_H

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

#include "m68k_arg.h"
#include "m68k_error_messages.h"
#include "m68k_insn_validate.h"
#include "expr/expr_fits.h"

namespace kas::m68k::opc
{
using namespace meta;

// validate function signature: pointer to function returning `op_size_t`
using expr_fits   = expression::expr_fits;
using fits_result = expression::fits_result;
using op_size_t   = core::opcode::op_size_t;

// declare addressing mode constants
enum {
      AM_GEN        = 1 <<  0
    , AM_DATA       = 1 <<  1
    , AM_MEMORY     = 1 <<  2
    , AM_CTRL       = 1 <<  3 
    , AM_ALTERABLE  = 1 <<  4
    , AM_IMMED      = 1 <<  5
    , AM_DIRECT     = 1 <<  6
    , AM_REGSET     = 1 <<  7
    , AM_PDEC       = 1 <<  8
    , AM_PINC       = 1 <<  9
    , AM_INDIRECT   = 1 << 10
    , AM_GEN_REG    = 1 << 11
    , AM_NONE       = 1 << 15
    };


// use preprocessor to define string names used in definitions & debugging...
#define VAL(N, AM)       using N = meta::list<KAS_STRING(#N), val_am,  meta::int_<AM>>
#define VAL_REG(N, ...)  using N = _val_reg<KAS_STRING(#N), __VA_ARGS__>

// validate based on "access mode"
struct val_am : m68k_mcode_t::val_t
{
    constexpr val_am(uint16_t am) : match {am} {}

    // test argument against validation
    fits_result ok(m68k_arg_t& arg, m68k_size_t sz, expr_fits const& fits) const override
    {
        // basic AM mode match
        if ((arg.am_bitset() & match) != match)
            return fits.no;

        // translate immed arg to include `size`
        if (arg.mode() == MODE_IMMED)
            arg.set_mode(static_cast<arg_mode_t>(MODE_IMMED_BASE + sz));

        // disallow PC-relative if arg is "ALTERABLE"
        if (arg.mode() == MODE_DIRECT)
            if (match & AM_ALTERABLE)
                arg.set_mode(MODE_DIRECT_ALTER);

        // if expression doesn't fit, will error out later.
        // better matches show up in size
        return fits.yes;
    }

    // bytes required by arg: registers never require extra space, but "extensions" may..
    fits_result size(m68k_arg_t& arg, m68k_size_t sz, expr_fits const& fits, op_size_t *size_p) const override
    {
        // regset baked into pudding
        if (match == AM_REGSET)
            return fits.yes;

        auto arg_size = arg.size(fits);
        if (arg_size.is_error())
            return fits.no;

        *size_p += arg_size;

        // if `arg_size` not fixed, it's a maybe.
        auto result = fits.yes;
        if (arg_size.min != arg_size.max)
            result = fits.maybe;

        // infer "maybe": if `inner` or `outer` not resolved, it's a maybe.
        if (result == fits.yes)
            if (fits.fits<int16_t>(arg.expr) == fits.maybe)
                result = fits.maybe;
        if (result == fits.yes)
            if (fits.fits<int16_t>(arg.outer) == fits.maybe)
                result = fits.maybe;

        // use arg generic routine to see if coldfire limit exceeded
        return coldfire_limit(result, size_p);
    }

    uint16_t match;
};

// validate based on "register class" or specific "register"
struct val_reg : m68k_mcode_t::val_t
{
    constexpr val_reg(uint16_t r_class, uint16_t r_num = ~0) : r_class{r_class}, r_num(r_num) {}

    // test argument against validation
    fits_result ok(m68k_arg_t& arg, m68k_size_t sz, expr_fits const& fits) const override
    {
        // must special case ADDR_REG & DATA_REG as these are
        // stored as a "arg.mode()": (magic number alert...).
        if (r_class <= RC_ADDR)
            return (r_class == arg.mode()) ? fits.yes : fits.no;

        // Other register classes are coded as mode = MODE_REG
        else if (arg.mode() != MODE_REG)
            return fits.no;

        // here validating if MODE_REG arg matches. validate if REG class matches desired
        else
        {
           auto reg_p = arg.expr.template get_p<m68k_reg_t>();
           if (reg_p->kind() != r_class)
                return fits.no;
        } 

        // here reg-class matches. Test reg-num if specified
        // test if testing for rc_class only (ie. rc_value == ~0)
        if (r_num == static_cast<decltype(r_num)>(~0))
            return fits.yes;

        // not default: look up actual rc_value
        auto reg_p = arg.expr.template get_p<m68k_reg_t>();
        if (reg_p->value() == r_num)
            return fits.yes;

        return fits.no;
    }
    
    // registers by themselves have no size. Don't override default size() method 

    uint16_t r_class, r_num;
};

template <typename N, int...Ts>
using _val_reg = meta::list<N, val_reg, meta::int_<Ts>...>;


VAL(GEN,            AM_GEN);
VAL(ALTERABLE,      AM_ALTERABLE);
VAL(DATA,           AM_DATA);
VAL(DATA_ALTER,     AM_DATA | AM_ALTERABLE);
VAL(CONTROL,        AM_CTRL);
VAL(CONTROL_ALTER,  AM_CTRL | AM_ALTERABLE);
VAL(MEM,            AM_MEMORY);
VAL(MEM_ALTER,      AM_MEMORY | AM_ALTERABLE);
VAL(IMMED,          AM_IMMED);
VAL(POST_INCR,      AM_PINC);       // mode == 3
VAL(PRE_DECR,       AM_PDEC);       // mode == 4
VAL(DIRECT,         AM_DIRECT);     // mode == 7/0 & 7/1
VAL(INDIRECT,       AM_INDIRECT);   // mode == 2/3/4/5
VAL(GEN_REG,        AM_GEN_REG);    // mode == 0/1
VAL(CONTROL_INDIR,  AM_CTRL | AM_INDIRECT);  // mode = 2/5

// register-class and register-specific validations (reg_class must be < 13)
VAL_REG(DATA_REG,   RC_DATA);
VAL_REG(ADDR_REG,   RC_ADDR);
VAL_REG(CTRL_REG,   RC_CTRL);
VAL_REG(FP_REG,     RC_FLOAT);
VAL_REG(FCTRL_REG,  RC_FCTRL);

VAL_REG(FPIAR,      RC_FCTRL, REG_FPCTRL_IAR);
VAL_REG(SR,         RC_CPU,   REG_CPU_SR);
VAL_REG(CCR,        RC_CPU,   REG_CPU_CCR);
VAL_REG(USP,        RC_CPU,   REG_CPU_USP);

// coldfire MAC adds many named registers...
VAL_REG(ACC,        RC_CPU,   REG_CPU_ACC);
VAL_REG(MACSR,      RC_CPU,   REG_CPU_MACSR);
VAL_REG(MASK,       RC_CPU,   REG_CPU_MASK);
VAL_REG(SF_LEFT,    RC_CPU,   REG_CPU_SF_LEFT);
VAL_REG(SF_RIGHT,   RC_CPU,   REG_CPU_SF_RIGHT);
VAL_REG(ACC_EXT01,  RC_CPU,   REG_CPU_ACC_EXT01);
VAL_REG(ACC_EXT23,  RC_CPU,   REG_CPU_ACC_EXT23);

#undef VAL
#undef VAL_REG

}


namespace kas::m68k
{

// NB: _am_bitset is `mutable` so this calculation is done once...
inline uint16_t m68k_arg_t::am_bitset() const
{
    using namespace opc;        // pickup AM_* definitions

    // magic numbers below are from m68k opcode formats...
    static constexpr uint16_t cpu_mode_to_access_mode[] = {
          AM_GEN | AM_DATA                       | AM_ALTERABLE | AM_GEN_REG    // 0: data_reg
        , AM_GEN                                 | AM_ALTERABLE | AM_GEN_REG    // 1: addr_reg
        , AM_GEN | AM_DATA | AM_MEMORY | AM_CTRL | AM_ALTERABLE | AM_INDIRECT   // 2: addr_indirect
        , AM_GEN | AM_DATA | AM_MEMORY | AM_PINC | AM_ALTERABLE | AM_INDIRECT   // 3: post_incr
        , AM_GEN | AM_DATA | AM_MEMORY | AM_PDEC | AM_ALTERABLE | AM_INDIRECT   // 4: pre_decr
        , AM_GEN | AM_DATA | AM_MEMORY | AM_CTRL | AM_ALTERABLE | AM_INDIRECT   // 5: addr_displacement
        , AM_GEN | AM_DATA | AM_MEMORY | AM_CTRL | AM_ALTERABLE                 // 6: index
        , AM_GEN | AM_DATA | AM_MEMORY | AM_CTRL | AM_ALTERABLE | AM_DIRECT     // 7-0: abs word
        , AM_GEN | AM_DATA | AM_MEMORY | AM_CTRL | AM_ALTERABLE | AM_DIRECT     // 7-1: abs long
        , AM_GEN | AM_DATA | AM_MEMORY | AM_CTRL                                // 7-2: pc displacement
        , AM_GEN | AM_DATA | AM_MEMORY | AM_CTRL                                // 7-3: pc index
        , AM_GEN | AM_DATA | AM_MEMORY | AM_IMMED | AM_REGSET                   // 7-4: immed
        };

    if (!_am_bitset) {
        unsigned am_index = mode_normalize();
        switch (am_index) {
            case MODE_REGSET:
                return _am_bitset = AM_REGSET;
            default:
                // OK if directly-mapped mode...
                if (am_index <= MODE_IMMED)
                    // ...and NOT special coldfire MAC mode
                    if (reg_subword == REG_SUBWORD_FULL)
                        break;
                        
                // ERROR: no match. Pick non-zero value which never matches
                return _am_bitset = AM_NONE;
        }
        _am_bitset = cpu_mode_to_access_mode[am_index];

    }
    return _am_bitset;
}
}
#endif
