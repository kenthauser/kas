#ifndef KAS_Z80_VALIDATE_REG_H
#define KAS_Z80_VALIDATE_REG_H

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

#include "z80_arg_defn.h"
#include "z80_error_messages.h"
#include "z80_insn_validate.h"
#include "expr/expr_fits.h"




namespace kas::z80::opc
{
using namespace meta;

// validate function signature: pointer to function returning `op_size_t`
using expr_fits   = expression::expr_fits;
using fits_result = expression::fits_result;
using op_size_t   = core::opcode::op_size_t;

// use preprocessor to define string names used in definitions & debugging...
#define VAL_REG(N, ...) using N = _val_reg<KAS_STRING(#N), __VA_ARGS__>
#define VAL_GEN(N, ...) using N = _val_gen<KAS_STRING(#N), __VA_ARGS__>

// validate based on "register class" or specific "register"
struct val_reg : z80_validate
{
    constexpr val_reg(uint16_t r_class, uint16_t r_num = ~0) : r_class{r_class}, r_num(r_num) {}

    // test argument against validation
    fits_result ok(z80_arg_t& arg, expr_fits const& fits) const override
    {
        // Other register classes are coded as mode = MODE_REG
        if (arg.mode != MODE_REG)
            return fits.no;

        // here validating if MODE_REG arg matches. validate if REG class matches desired
       if (arg.reg.kind(r_class) != r_class)
            return fits.no;

        // here reg-class matches. Test reg-num if specified
        // test if testing for rc_class only (ie. rc_value == ~0)
        if (r_num == static_cast<decltype(r_num)>(~0))
            return fits.yes;

        // not default: look up actual rc_value
        if (arg.reg.value() == r_num)
            return fits.yes;

        return fits.no;
    }
    
    // registers by themselves have no size. Don't override default size() method 

    uint16_t r_class, r_num;
};
    
struct val_jrcc : z80_validate
{
    constexpr val_jrcc() {}

    // test argument against validation
    fits_result ok(z80_arg_t& arg, expr_fits const& fits) const override
    {
        if (arg.mode != MODE_REG)
            return fits.no;
        if (arg.reg.kind() != RC_CC)
            return fits.no;
        if (arg.reg.value() >= 4)
            return fits.no;
        return fits.yes;
    }
};

// the "range" validators all resolve to zero size
struct val_range : z80_validate
{
    constexpr val_range(int32_t min, int32_t max) : min(min), max(max) {}

    fits_result ok(z80_arg_t& arg, expr_fits const& fits) const override
    {
        // range is only for direct args
        if (arg.mode == MODE_DIRECT)
            return fits.fits(arg.expr, min, max);
    
        return fits.no;
    }

    int32_t min, max;
};

struct val_restart : z80_validate
{
    constexpr val_restart() {}

    fits_result ok(z80_arg_t& arg, expr_fits const& fits) const override
    {
        // range is only for direct args
        if (arg.mode != MODE_DIRECT)
            return fits.no;
        
        auto p = arg.expr.get_fixed_p();

        // require fixed argument
        if (!p)
            return fits.no;

        // require multiple of 8, range 0..0x38
        if (~(7 << 3) & *p)
            return fits.no;
    
        return fits.yes;
    }
};

struct val_indir : z80_validate
{
    constexpr val_indir(int rc = -1, int r_num = ~0) : rc(rc), r_num(r_num) {}

    fits_result ok(z80_arg_t& arg, expr_fits const& fits) const override
    {
        return fits.no;
    }

    int rc;
    uint16_t r_num;
};

template <typename N, int...Ts>
using _val_reg = meta::list<N, val_reg, meta::int_<Ts>...>;

template <typename N, typename T, int...Ts>
using _val_gen = list<N, T, int_<Ts>...>;

// register-class and register-specific validations
VAL_REG(REG         , RC_GEN);
VAL_REG(REG_GEN     , RC_GEN);      // XXX
VAL_REG(REG_A       , RC_GEN, 7);
VAL_REG(REG_C       , RC_GEN, 1);
VAL_REG(REG_I       , RC_I);
VAL_REG(REG_R       , RC_R);

VAL_REG(REG_DBL     , RC_DBL);
VAL_REG(REG_DBL_AF  , RC_DBL);
VAL_REG(REG_IDX     , RC_IDX);
VAL_REG(IDX_HL      , RC_IDX);  // XXX
VAL_REG(REG_AF      , RC_AF);
VAL_REG(REG_BC      , RC_DBL, 0);
VAL_REG(REG_DE      , RC_DBL, 1);
VAL_REG(REG_HL      , RC_DBL, 2);
VAL_REG(REG_SP      , RC_SP);
VAL_REG(REG_IX      , RC_IDX, 0xdd);
VAL_REG(REG_IY      , RC_IDX, 0xfd);

VAL_REG(CC          , RC_CC);
VAL_GEN(JR_CC       , val_jrcc);
VAL_GEN(DIRECT      , val_restart); // XXX

VAL_GEN(IMMED_8     , val_range, -(1<< 7), (1<< 8)-1);
VAL_GEN(IMMED_16    , val_range, -(1<<15), (1<<16)-1);
VAL_GEN(IMMED_012   , val_range, 0, 2);
VAL_GEN(BIT_NUM     , val_range, 0, 7);
VAL_GEN(IMMED_RST   , val_restart);

VAL_GEN(INDIR       , val_indir);
VAL_GEN(INDIR_HL    , val_indir, RC_IDX);
VAL_GEN(INDIR_SP    , val_indir, RC_SP);
VAL_GEN(INDIR_DBL   , val_indir, RC_DBL);
VAL_GEN(INDIR_C     , val_indir, RC_GEN, 1);
}

#undef VAL_REG
#undef VAL_GEN
#endif
