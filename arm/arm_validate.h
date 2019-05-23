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
#include "target/tgt_validate.h"


namespace kas::arm::opc
{
using expr_fits   = expression::expr_fits;
using fits_result = expression::fits_result;
using op_size_t   = core::opcode::op_size_t;

// Derive common validators from generic templates

struct val_reg : tgt::opc::tgt_val_reg<val_reg, arm_mcode_t>
{
    using base_t::base_t;
};

struct val_range : tgt::opc::tgt_val_range<val_range, arm_mcode_t>
{
    using base_t::base_t;
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

// Named Registers
VAL_REG(SP          , RC_GEN, 13);
VAL_REG(LR          , RC_GEN, 14);
VAL_REG(PC          , RC_GEN, 15);

VAL_GEN(IMM24       , val_range, 0, 0, (1<<12) - 1);
VAL_GEN(IMM16       , val_range, 0, 0, (1<<12) - 1);
VAL_GEN(IMM12       , val_range, 0, 0, (1<<12) - 1);
VAL_GEN(IMM5        , val_range, 0, 0, (1<<5 ) - 1);
VAL_GEN(IMM4        , val_range, 0, 0, (1<<5 ) - 1);

VAL_GEN(ZERO        , val_range, 0, 0, 0);
VAL_GEN(LABEL       , val_range, 0, 0, (1<<12) - 1);
VAL_GEN(SHIFT       , val_range, 0, 0, 0);
VAL_GEN(SHIFT_Z     , val_range, 0, 0, 0);
VAL_GEN(SHIFT_NZ    , val_range, 0, 0, 0);
VAL_GEN(REG_INDIR   , val_range, 0, 0, 0);
VAL_GEN(REGSET      , val_range, 0, 0, 0);
VAL_GEN(REG_OFFSET  , val_range, 0, 0, 0);
VAL_GEN(REG_UPDATE  , val_range, 0, 0, 0);
VAL_GEN(SP_UPDATE   , val_range, 0, 0, 0);

VAL_GEN(IFLAGS      , val_range, 0, 0, 0);
VAL_GEN(ENDIAN      , val_range, 0, 0, 0);
VAL_GEN(ONES        , val_range, 0, 0, 0);
VAL_GEN(DMB_OPTION  , val_range, 0, 0, 0);
VAL_GEN(ISB_OPTION  , val_range, 0, 0, 0);
}

#undef VAL_REG
#undef VAL_GEN
#endif
