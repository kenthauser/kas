#ifndef KAS_PROTO_UC_VALIDATE_H
#define KAS_PROTO_UC_VALIDATE_H

/******************************************************************************
 *
 * Instruction argument validation.
 *
 * See `target/tgt_validate.h` for information on virtual base class
 *
 *****************************************************************************/

#include "PROTO_LC_mcode.h"
#include "target/tgt_validate_generic.h"
#include "target/tgt_validate_branch.h"


namespace kas::PROTO_LC::opc
{
using expr_fits   = expression::expr_fits;
using fits_result = expression::fits_result;
using op_size_t   = core::opcode::op_size_t;

// Derive common validators from generic templates

struct val_reg : tgt::opc::tgt_val_reg<val_reg, PROTO_LC_mcode_t>
{
    using base_t::base_t;
};

struct val_branch : tgt::opc::tgt_val_branch<val_branch, PROTO_LC_mcode_t>
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
VAL_REG(REG         , RC_GEN);      // Named Registers
VAL_REG(REG_SP      , RC_GEN, 7);   // Specified Register
VAL_REG(REG_PC      , RC_CPU, 0);
}

#undef VAL_REG
#undef VAL_GEN
#endif
