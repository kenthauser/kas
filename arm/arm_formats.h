#ifndef KAS_ARM_ARM_FORMATS_H
#define KAS_ARM_ARM_FORMATS_H

// Declare ARM argument inserters.
// See `target/tgt_format.h` for naming convention.

#include "arm_formats_impl.h"
#include "arm_formats_opc.h"

namespace kas::arm::opc
{
// declare mixin classes to override virtual functions
// override insert/extract pairs together...
// NB: fmt_generic args are: shift, width, word = 0

// insert/extract from first word (often 4-bit register/value)
using gen_00b4 = fmt32_generic< 0, 4>;
using gen_04b4 = fmt32_generic< 4, 4>;
using gen_08b4 = fmt32_generic< 8, 4>;
using gen_12b4 = fmt32_generic<12, 4>;
using gen_16b4 = fmt32_generic<16, 4>;
using gen_20b4 = fmt32_generic<20, 4>;

// register/value inserters: declare 4-bit locations for several args
using arg1_00b4 = fmt_arg<1, gen_00b4>;
using arg1_04b4 = fmt_arg<1, gen_04b4>;
using arg1_08b4 = fmt_arg<1, gen_08b4>;
using arg1_12b4 = fmt_arg<1, gen_12b4>;
using arg1_16b4 = fmt_arg<1, gen_16b4>;
using arg1_20b4 = fmt_arg<1, gen_20b4>;

using arg2_00b4 = fmt_arg<2, gen_00b4>;
using arg2_12b4 = fmt_arg<2, gen_12b4>;
using arg2_16b4 = fmt_arg<2, gen_16b4>;
using arg2_20b4 = fmt_arg<2, gen_20b4>;

using arg3_00b4 = fmt_arg<3, gen_00b4>;
using arg3_08b4 = fmt_arg<3, gen_08b4>;
using arg3_12b4 = fmt_arg<3, gen_12b4>;
using arg3_16b4 = fmt_arg<3, gen_16b4>;
using arg3_20b4 = fmt_arg<3, gen_20b4>;

using arg4_00b4 = fmt_arg<4, gen_00b4>;
using arg4_08b4 = fmt_arg<4, gen_08b4>;
using arg4_12b4 = fmt_arg<4, gen_12b4>;
using arg4_16b4 = fmt_arg<4, gen_16b4>;
using arg4_20b4 = fmt_arg<4, gen_20b4>;

using arg5_00b4 = fmt_arg<5, gen_00b4>;

// declare immed for arg 2+
using arg2_fixed   = fmt_arg<2, fmt_fixed>;
using arg3_fixed   = fmt_arg<3, fmt_fixed>;
using arg4_fixed   = fmt_arg<4, fmt_fixed>;

// declare shifter for arg 2+
using arg2_shifter = fmt_arg<2, fmt_shifter>;
using arg3_shifter = fmt_arg<3, fmt_shifter>;
using arg4_shifter = fmt_arg<4, fmt_shifter>;

//
// Declare types used in INSN definitions
//

// conventional name for format used by `OPC_LIST`
struct FMT_LIST : fmt_list, arg1_12b4 {};

// conventional name for `no-args` formatter
struct FMT_X     : fmt_gen {};

// Branches:
// XXX insert 24-bit offset. need to handle out-of-range case
#if 0
struct FMT_B     : fmt_gen, fmt_arg<1, fmt32_generic<0, 24>> {};
struct FMT_BT    : fmt_gen, fmt_arg<1, fmt32_generic<0, 25>> {};
#else
using FMT_B = FMT_X;
using FMT_BT = FMT_X;
#endif

// single arg formats
struct FMT_0     : fmt_gen, arg1_00b4 {};
struct FMT_12    : fmt_gen, arg1_12b4 {};
struct FMT_16    : fmt_gen, arg1_16b4 {};

// support ARM5 addressing mode 1
struct FMT_12_16     : fmt_gen, arg1_12b4, arg2_16b4 {};
struct FMT_12_16_F   : FMT_12_16, arg3_fixed         {};
struct FMT_12_16_S   : FMT_12_16, arg3_shifter       {}; 
struct FMT_12_16_0   : FMT_12_16, arg3_00b4          {};
struct FMT_12_16_0_S : FMT_12_16_0, arg4_shifter     {};

// addressing mode 1: no Rn (eg mov)
struct FMT_12_0   : fmt_gen, arg1_12b4, arg2_00b4     {};
struct FMT_12_0_S : FMT_12_0, arg3_shifter            {};
struct FMT_12_F   : fmt_gen, arg1_12b4, arg2_fixed    {};

// addressing mode 1: no Rd (eg cmp)`
struct FMT_16_0   : fmt_gen, arg1_16b4, arg2_00b4  {};
struct FMT_16_0_S : FMT_16_0, arg3_shifter         {};
struct FMT_16_F   : fmt_gen, arg1_16b4, arg2_fixed {};

// support ARM5 addressing mode 2
struct FMT_LD  : fmt_gen, arg1_12b4, arg2_shifter {};
struct FMT_PLD : FMT_X {};

// support ARM5 addressing mode 3
struct FMT_LDH : FMT_X {};
struct FMT_STH : FMT_X {};

// `srs`: 5 LSBs are immed value
struct FMT_I00B5 : fmt_gen, fmt_arg<1, fmt32_generic<0, 5>> {};

// addressing mode 4
struct FMT_LDM : FMT_X {};
struct FMT_STM : FMT_X {};
struct FMT_8_12_M4 : FMT_X {};
struct FMT_8_20_12_16_0 : fmt_gen, arg1_08b4, arg2_20b4, arg3_12b4, arg4_16b4, arg5_00b4 {};
struct FMT_8_20_12_16_0_05B3 : FMT_8_20_12_16_0, fmt_arg<6, fmt32_generic<5, 3>> {};

// unconditional instructions
struct FMT_4_0  : fmt_gen, arg1_04b4, arg2_00b4 {};
struct FMT_I09B1 : fmt_gen, fmt_arg<1, fmt32_generic<9, 1>> {};

// multiply instructions
struct FMT_16_0_8 : fmt_gen, arg1_16b4, arg2_00b4, arg3_08b4 {};
struct FMT_16_0_8_12 : FMT_16_0_8, arg4_12b4 {};

struct FMT_12_16_0_8 : FMT_12_16_0, arg4_08b4 {};

// ssat, et al (5-bit shift count at bit 16)
struct FMT_12_B_0_S : fmt_gen, arg1_12b4, fmt_arg<2, fmt32_generic<16, 5>>
                            , arg3_00b4, arg4_shifter {};

// xtnd instructions
struct FMT_12_16_0_8B2 : FMT_12_16_0, fmt_arg<4, fmt32_generic<8, 2>> {};
struct FMT_12_0_8B2    : FMT_12_0   , fmt_arg<3, fmt32_generic<8, 2>> {};

// swp, swpb
struct FMT_12_0_16 : fmt_gen, arg1_12b4, arg2_00b4, arg3_16b4 {};

// SWI, BKPT
struct FMT_I24     : fmt_gen, fmt_arg<1, fmt32_generic<0, 24>> {};
struct FMT_BKPT    : FMT_I24 {};    // XXX

//struct FMT_F       : fmt_gen::fmt_t {};     // empty for now 
//using arg1_f  = fmt_arg<1, typename fmt_gen::fmt_t>;
//using arg2_f  = fmt_arg<2, typename fmt_gen::fmt_t>;
//using arg3_f  = fmt_arg<3, typename fmt_gen::fmt_t>;

#if 0
// JR uses special OPCODE to generate machine code
struct FMT_JR      : fmt_jr           {};
struct FMT_JRCC    : fmt_jr, arg1_3b2 {};
struct FMT_DJNZ    : fmt_djnz         {};

struct FMT_12_F    : fmt_gen, arg1_12b4, arg2_fixed {};
struct FMT_12_0    : fmt_gen, arg1_12b4, arg2_00b4 {};
struct FMT_12_0_F  : fmt_gen, arg1_12b4, arg2_00b4, arg3_fixed {};
struct FMT_12_MOVW : fmt_gen, arg1_12b4, fmt_arg<2, fmt_movw> {};
#endif
#if 0
// general registers are 3-bits shifted 0 or 3 bits
struct FMT_0       : fmt_gen, arg1_0b3 {};
struct FMT_X_0     : fmt_gen, arg2_0b3 {};
struct FMT_3_0     : fmt_gen, arg1_3b3, arg2_0b3 {};
struct FMT_3       : fmt_gen, arg1_3b3 {};
struct FMT_X_3     : fmt_gen, arg2_3b3 {};

// IM: 2 bits modified & shifed 3 bits in second word
struct FMT_1W3B2   : fmt_gen, arg1_1w3b2 {};

// DBL registers are two bits shifted 4
struct FMT_4       : fmt_gen, arg1_4b2 {};
struct FMT_X_4     : fmt_gen, arg2_4b2 {};

// BC/DE instructions: 1 bit shifted 4
struct FMT_4B1     : fmt_gen, arg1_4b1 {};
struct FMT_X_4B1   : fmt_gen, arg2_4b1 {};

// CB instructions
struct FMT_1W4     : fmt_gen, arg1_1w4b2 {};
struct FMT_X_1W4   : fmt_gen, arg2_1w4b2 {};
struct FMT_1W0     : fmt_gen, arg1_1w0b3 {};
struct FMT_1W3     : fmt_gen, arg1_1w3b3 {};
struct FMT_X_1W3   : fmt_gen, arg2_1w3b3 {};
struct FMT_1W3_1W0 : fmt_gen, arg1_1w3b3, arg2_1w0b3 {};

#endif
}
#endif




