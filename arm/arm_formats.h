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

// insert/extract from first word
using gen_00b4 = fmt_generic< 0, 4>;
using gen_12b4 = fmt_generic<12, 4>;
using gen_16b4 = fmt_generic<16, 4>;
using gen_20b4 = fmt_generic<20, 4>;

#if 0
using gen_4b2 = fmt_generic<4, 2>;
using gen_4b1 = fmt_generic<4, 1>;

// insert/extract from second word
using gen_1w0b3 = fmt_generic<0, 3, 1>;
using gen_1w3b3 = fmt_generic<3, 3, 1>;
using gen_1w3b2 = fmt_generic<3, 2, 1>;
using gen_1w4b2 = fmt_generic<4, 2, 1>;
#endif

// bind `inserters` with `arg num`
using arg1_fixed = fmt_arg<1, fmt_fixed>;
using arg2_fixed = fmt_arg<2, fmt_fixed>;
using arg3_fixed = fmt_arg<3, fmt_fixed>;
using arg4_fixed = fmt_arg<4, fmt_fixed>;


using arg1_00b4 = fmt_arg<1, gen_00b4>;
using arg1_12b4 = fmt_arg<1, gen_12b4>;
using arg1_16b4 = fmt_arg<1, gen_16b4>;
using arg1_20b4 = fmt_arg<1, gen_20b4>;

using arg2_00b4 = fmt_arg<2, gen_00b4>;
using arg2_12b4 = fmt_arg<2, gen_12b4>;
using arg2_16b4 = fmt_arg<2, gen_16b4>;
using arg2_20b4 = fmt_arg<2, gen_20b4>;

using arg3_00b4 = fmt_arg<3, gen_00b4>;
using arg3_12b4 = fmt_arg<3, gen_12b4>;
using arg3_16b4 = fmt_arg<3, gen_16b4>;
using arg3_20b4 = fmt_arg<3, gen_20b4>;

using arg4_00b4 = fmt_arg<4, gen_00b4>;
using arg4_12b4 = fmt_arg<4, gen_12b4>;
using arg4_16b4 = fmt_arg<4, gen_16b4>;
using arg4_20b4 = fmt_arg<4, gen_20b4>;

#if 0
// second word args
using arg1_1w0b3  = fmt_arg<1, gen_1w0b3>;
using arg1_1w3b3  = fmt_arg<1, gen_1w3b3>;
using arg2_1w0b3  = fmt_arg<2, gen_1w0b3>;
using arg2_1w3b3  = fmt_arg<2, gen_1w3b3>;

using arg1_1w3b2  = fmt_arg<1, gen_1w3b2>;
using arg2_1w3b2  = fmt_arg<2, gen_1w3b2>;

using arg1_1w4b2  = fmt_arg<1, gen_1w4b2>;
using arg2_1w4b2  = fmt_arg<2, gen_1w4b2>;
#endif
//
// Declare types used in INSN definitions
//

// conventional name for format used by `OPC_LIST`
// NB: VALIDATORs are REG_GEN, REG_GEN
// NB: purposefully make unique (arg order doesn't match move)
//struct FMT_LIST     : fmt_list, arg1_0b3, arg2_3b3 {};
struct FMT_LIST : fmt_list {};

// conventional name for `no-args` formatter
struct FMT_X       : fmt_gen {};

struct FMT_0       : fmt_gen, arg1_00b4 {};
struct FMT_S   : FMT_X {};
struct FMT_16_S : FMT_X {};
struct FMT_16_RS : FMT_X {};
struct FMT_12_S : FMT_X {};
struct FMT_12_RS : FMT_X {};
struct FMT_LD  : FMT_X {};
struct FMT_ST  : FMT_X {};
struct FMT_PLD : FMT_X {};
struct FMT_LDH : FMT_X {};
struct FMT_STH : FMT_X {};
struct FMT_LDM : FMT_X {};
struct FMT_STM : FMT_X {};

//struct FMT_F       : fmt_gen::fmt_t {};     // empty for now 
//using arg1_f  = fmt_arg<1, typename fmt_gen::fmt_t>;
//using arg2_f  = fmt_arg<2, typename fmt_gen::fmt_t>;
//using arg3_f  = fmt_arg<3, typename fmt_gen::fmt_t>;

#if 0
// JR uses special OPCODE to generate machine code
struct FMT_JR      : fmt_jr           {};
struct FMT_JRCC    : fmt_jr, arg1_3b2 {};
struct FMT_DJNZ    : fmt_djnz         {};
#endif

struct FMT_12_F    : fmt_gen, arg1_12b4, arg2_fixed {};
struct FMT_12_0    : fmt_gen, arg1_12b4, arg2_00b4 {};
struct FMT_12_0_F  : fmt_gen, arg1_12b4, arg2_00b4, arg3_fixed {};
struct FMT_12_MOVW : fmt_gen, arg1_12b4, fmt_arg<2, fmt_movw> {};
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




