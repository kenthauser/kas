#ifndef KAS_ARM_VALIDATE_H
#define KAS_ARM_VALIDATE_H

/******************************************************************************
 *
 * Instruction argument validation.
 *
 * See `target/tgt_validate.h` for information on virtual base class
 *
 *****************************************************************************/

#include "arm_mcode.h"
#include "target/tgt_validate_generic.h"
#include "target/tgt_validate_branch.h"


namespace kas::arm::opc
{
using expr_fits   = expression::expr_fits;
using fits_result = expression::fits_result;
using op_size_t   = core::opcode::op_size_t;

// Derive common validators from generic templates

struct val_reg : tgt::opc::tgt_val_reg<val_reg, arm_mcode_t>
{
    using base_t::base_t;

    fits_result ok(arm_arg_t& arg, expr_fits const& fits) const override
    {
        if (arg.mode() != MODE_REG)
            return fits.no;
        if (arg.reg_p->kind(RC_GEN) != RC_GEN)
            return fits.no;
        return fits.yes;
    }

    // standard ARM encoding
    unsigned get_value(arm_arg_t& arg) const override
    {
        return arg.reg_p->value(RC_GEN);
    }

    void set_arg(arm_arg_t& arg, unsigned value) const override
    {
        arg.reg_p = &arm_reg_t::find(RC_GEN, value);
    }
};

struct val_range: tgt::opc::tgt_val_range<arm_mcode_t, int32_t>
{
    using base_t::base_t;
};

 
struct val_shift : arm_mcode_t::val_t
{
    constexpr val_shift() {}

    fits_result ok(arm_arg_t& arg, expr_fits const& fits) const override
    {
        return arg.mode() == MODE_SHIFT ? fits.yes : fits.no;
    }

    // standard ARM encoding
    unsigned get_value(arm_arg_t& arg) const override
    {
        if (arg.shift.is_reg)
            return (arg.shift.type << 5) | (arg.shift.ext << 8) | 0x10;    
        return (arg.shift.type << 5) | ((arg.shift.ext & 0x1f) << 7);

    }

    void set_arg(arm_arg_t& arg, unsigned value) const override
    {
        // extract "shift" from opcode.
        arg.shift.is_reg = !!(value & 0x10);
        value >>= 5;
        arg.shift.type = value & 3;
        value >>= 2;
        value &=  0x1f;
        if (arg.shift.is_reg)
            arg.shift.ext = value >> 1;
        else 
            arg.shift.ext = value;
    }
};

struct val_indir : arm_mcode_t::val_t
{
    constexpr val_indir() {}

    fits_result ok(arm_arg_t& arg, expr_fits const& fits) const override
    {
        return arg.mode() == MODE_REG_INDIR ? fits.yes : fits.no;
    }

    // standard ARM encoding
    unsigned get_value(arm_arg_t& arg) const override
    {
        auto value  = arg.indir.flags & 0x3f;
         
        value <<= 4;
        value += arg.reg_p->value(RC_GEN);
        value <<= 16;

        // if "register" format
        if (arg.indir.flags & arm_indirect::R_FLAG)
        {
            value += arg.indir.reg;
            if (arg.indir.flags & arm_indirect::S_FLAG)
                value += val_shift().get_value(arg);
        }

        // else "immed" format
        else
        {
            if (auto p = arg.expr.get_fixed_p())
                value += *p;
            else
                ; // deal with relocs.
        }

        std::cout << "indir::get_value() -> " << std::hex << value << std::endl;
        
        return value;
    }

    void set_arg(arm_arg_t& arg, unsigned value) const override
    {
    }
};
 

// use preprocessor to define string names used in definitions & debugging...
#define VAL_REG(NAME, ...) using NAME = _val_reg<KAS_STRING(#NAME), __VA_ARGS__>
#define VAL_GEN(NAME, ...) using NAME = _val_gen<KAS_STRING(#NAME), __VA_ARGS__>

template <typename NAME, typename T, int...Ts>
using _val_gen = meta::list<NAME, T, meta::int_<Ts>...>;

template <typename NAME, int...Ts>
using _val_reg = _val_gen<NAME, val_reg, Ts...>;

// register-class and register-specific validations
VAL_REG(REG         , RC_GEN);
VAL_REG(FLT_SGL     , RC_FLT_SGL);
VAL_REG(FLT_DBL     , RC_FLT_DBL);

VAL_REG(APSR        , RC_GEN);
VAL_REG(CPSR        , RC_GEN);
VAL_REG(SPSR        , RC_GEN);

// XXX DUMMY
VAL_REG(MEMORY      , RC_GEN);
VAL_REG(POST_INDEX  , RC_GEN);
VAL_REG(MEMORY_8    , RC_GEN);
VAL_REG(OFFSET12    , RC_GEN);


// Named Registers
VAL_REG(SP          , RC_GEN, 13);
VAL_REG(LR          , RC_GEN, 14);
VAL_REG(PC          , RC_GEN, 15);

// unsigned validators
VAL_GEN(U16         , val_range, 0, (1<<16) - 1);
VAL_GEN(U12         , val_range, 0, (1<<12) - 1);


VAL_GEN(IMM24       , val_range, 0, (1<<12) - 1);
VAL_GEN(IMM16       , val_range, 0, (1<<12) - 1);
VAL_GEN(IMM12       , val_range, 0, (1<<12) - 1);
VAL_GEN(IMM5        , val_range, 0, (1<<5 ) - 1);
VAL_GEN(IMM4        , val_range, 0, (1<<5 ) - 1);

VAL_GEN(ZERO        , val_range, 0, 0);
VAL_GEN(LABEL       , val_range, 0, (1<<12) - 1);
VAL_GEN(INDIR       , val_indir);
VAL_GEN(SHIFT       , val_shift);
VAL_GEN(SHIFT_Z     , val_range, 0, 0);
VAL_GEN(SHIFT_NZ    , val_range, 0, 0);
VAL_GEN(REGSET      , val_range, 0, 0);
VAL_GEN(REG_INDIR   , val_range, 0, 0);
VAL_GEN(REG_OFFSET  , val_range, 0, 0);
VAL_GEN(REG_UPDATE  , val_range, 0, 0);
VAL_GEN(SP_UPDATE   , val_range, 0, 0);

VAL_GEN(IFLAGS      , val_range, 0, 0);
VAL_GEN(ENDIAN      , val_range, 0, 0);
VAL_GEN(ONES        , val_range, 0, 0);
VAL_GEN(DMB_OPTION  , val_range, 0, 0);
VAL_GEN(ISB_OPTION  , val_range, 0, 0);
}

#undef VAL_REG
#undef VAL_GEN
#endif
