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

// break out special validators
// NB: "generic" validators in `validate_reg.h`
#include "arm_validate_reg.h"
#include "arm_validate_addr.h"
#include "arm_validate_special.h"

namespace kas::arm::opc
{

// use preprocessor to define string names used in definitions & debugging...
#define VAL_REG(NAME, ...) using NAME = _val_reg<KAS_STRING(#NAME), __VA_ARGS__>
#define VAL_GEN(NAME, ...) using NAME = _val_gen<KAS_STRING(#NAME), __VA_ARGS__>

template <typename NAME, typename T, int...Ts>
using _val_gen = meta::list<NAME, T, meta::int_<Ts>...>;

template <typename NAME, int...Ts>
using _val_reg = _val_gen<NAME, val_reg, Ts...>;

//
// Single register validators
//
// register-class and reg+mode validations
//

// ARM5 registers
VAL_REG(REG         , RC_GEN);
VAL_REG(REG_WB      , RC_GEN, val_reg::all_regs, arg_mode_t::MODE_REG_UPDATE);
VAL_REG(FLT_SGL     , RC_FLT_SGL);
VAL_REG(FLT_DBL     , RC_FLT_DBL);

VAL_REG(COPROC      , RC_COPROC);
VAL_REG(CREG        , RC_C_REG);

// Thumb register validators (only registers 0-7)
VAL_GEN(REGL        , val_regl);
VAL_GEN(REGL_WB     , val_regl_update);

// ARM5 Named Registers
VAL_REG(SP          , RC_GEN, 13);
VAL_REG(SP_WB       , RC_GEN, 13, arg_mode_t::MODE_REG_UPDATE);
VAL_REG(LR          , RC_GEN, 14);
VAL_REG(PC          , RC_GEN, 15);
VAL_GEN(NPC         , val_nopc);        // all RC_GEN except PC
VAL_REG(APSR        , RC_CPU, REG_CPU_APSR);
VAL_REG(CPSR        , RC_CPU, REG_CPU_CPSR);
VAL_REG(SPSR        , RC_CPU, REG_CPU_SPSR);

//
// Register-set validators
//

VAL_GEN(REGSET      , val_regset);
VAL_GEN(REGSET_SGL  , val_regset_single);
VAL_GEN(REGSET_USER , val_regset_user);

// special T16 regset validators
VAL_GEN(REGSET_T    , val_regset_l);
VAL_GEN(REGSET_T_LR , val_regset_l, 14);    // allow link register
VAL_GEN(REGSET_T_PC , val_regset_l, 15);    // allow pc

// 
// Immediate ARG validators
//
// All ARM constants are unsigned...
//

// simple validators 0..(2^^n)-1
VAL_GEN(U3      , val_range_u,  3);
VAL_GEN(U4      , val_range_u,  4);
VAL_GEN(U5      , val_range_u,  5);
VAL_GEN(U7      , val_range_u,  7);
VAL_GEN(U8      , val_range_u,  8);
VAL_GEN(U12     , val_range_u, 12);
VAL_GEN(U16     , val_range_u, 16);
VAL_GEN(U24     , val_range_u, 24);

// disallow zero shift count
//VAL_GEN(U5_NZ   , val_range, 1, 31);

// thumb immediates: fits in N bits after shift
//VAL_GEN(U7_4    , val_range_scaled<2>, 0, (1 << 7) - 1);
//VAL_GEN(U8_4    , val_range_scaled<2>, 0, (1 << 8) - 1);

// thumb saturating math: bit number: range 1 -> (1<<n), value -> (bit-1)
// XXX update FMT to not decrement value...
VAL_GEN(SAT4, val_range, 1, 16);     // XXX need -1 arg
VAL_GEN(SAT5, val_range, 1, 32);

    
//
// validate a32 branch and thumb branch displacements
// name by reloc generated. linker validates displacements
//

VAL_GEN(ARM_JUMP24  , val_arm_branch24);
VAL_GEN(ARM_CALL24  , val_arm_call24);
VAL_GEN(THB_JUMP8   , val_tmb_jump8);
VAL_GEN(THB_JUMP11  , val_tmb_jump11);

//
// ARM5 addressing mode validators
//

// ARM V5: addressing mode 1 validators
// accept any immediate value -- `kbfd` RELOC errors out invalid values
VAL_GEN(IMMED       , val_arg_mode, arg_mode_t::MODE_IMMEDIATE);
VAL_GEN(DIRECT      , val_direct);      // convert to PC-offset (via RELOC)
VAL_GEN(SHIFT       , val_shift);       // convert to/from 8-bit shift code

// ARM V5: addressing mode 2 validators
// NB: parser accepts general format of indirect.
// NB: validators convert to/from 16-LSBs for A32 insertion
VAL_GEN(A32_INDIR           , val_indir);
VAL_GEN(A32_POST_INDEX      , val_post_index);
VAL_GEN(A32_OFFSET12        , val_indir_offset);
VAL_GEN(A32_INDIR8_NOSHIFT  , val_ls_misc);

// special shifts for MISC insns. Return as standard shift 8-bit values.
VAL_GEN(ROR_B       , val_false);   // allow ROR of multiple of 8 bits
VAL_GEN(LSL         , val_false);   // allow LSR
VAL_GEN(ASR         , val_false);   // allow ASR

// coprocessor validators
VAL_GEN(CP_REG_INDIR, val_cp_indir);

// XXX allow a, i, f in any order
VAL_GEN(FLAGS_AIF    , val_false);

// XXX allow "ASPR_" prefix plus "nzcvq" suffixes or abbrev
// XXX allow "CSPR_" abbrev for ASPR
VAL_GEN(ASPR_BITS       , val_false);
VAL_GEN(CSPR_FIELDS     , val_false);
VAL_GEN(SPSR_FIELDS     , val_false);
VAL_GEN(FLAGS_ENDIAN    , val_false);  // allow BE (= 1), or LE (= 0)

// ARM V5: addressing mode 3 validators
//VAL_GEN(OFFSET8     , val_imm8);
#if 0

VAL_GEN(REGL_INDIR5_W_RELOC , val_indir_5, 2, true);    // relocatable
VAL_GEN(REGL_INDIR5_H       , val_indir_5, 1);
VAL_GEN(REGL_INDIR5_B       , val_indir_5, 0);
VAL_GEN(REGL_INDIRL , val_indir_l);
VAL_GEN(PC_INDIR8   , val_offset_8, 15);
VAL_GEN(SP_INDIR8   , val_offset_8, 13);
//
//
// 



VAL_GEN(LABEL       , val_range, 0, (1<<12) - 1);
VAL_GEN(SHIFT_Z     , val_false);
VAL_GEN(SHIFT_NZ    , val_false);
VAL_GEN(REG_OFFSET  , val_false);

VAL_GEN(ONES        , val_false);
VAL_GEN(DMB_OPTION  , val_false);
VAL_GEN(ISB_OPTION  , val_false);

VAL_GEN(BIT         , val_false);
VAL_GEN(BIT4        , val_false);

// THUMB32 Validators
VAL_GEN(THB_U12   , val_false);
VAL_GEN(OFFSET_21   , val_false);
VAL_GEN(OFFSET_25   , val_false);
VAL_GEN(BANKED_REG  , val_false);
VAL_GEN(SPEC_REG    , val_false);
VAL_GEN(IFLAGS      , val_false);
VAL_GEN(DSB_OPTION  , val_false);

VAL_GEN(INDIR_OFFSET8 , val_false);
VAL_GEN(REG_TBB     , val_false);
VAL_GEN(REG_TBH     , val_false);
VAL_GEN(REG_INDIR_W , val_false);
VAL_GEN(INDIR_OFFSET12, val_false);
#endif
}

#undef VAL_REG
#undef VAL_GEN
#endif
