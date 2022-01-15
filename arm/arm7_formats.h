#ifndef KAS_ARM_ARM7_FORMATS_H
#define KAS_ARM_ARM7_FORMATS_H

// Declare ARM argument inserters.
// See `target/tgt_format.h` for naming convention.

#include "arm_formats_opc.h"
#include "arm7_formats_ops.h"

namespace kas::arm::opc::a32
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

using gen_0b16 = fmt32_generic<0, 16>;  // register-set

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
// NB: these formatters use `kbfd` to process immediate args
using arg2_fixed   = fmt_arg<2, fmt_fixed>;
using arg3_fixed   = fmt_arg<3, fmt_fixed>;
using arg4_fixed   = fmt_arg<4, fmt_fixed>;
using arg3_addsub  = fmt_arg<3, fmt_addsub>;

// declare shifter for arg 2+
using arg2_shifter = fmt_arg<2, fmt_shifter>;
using arg3_shifter = fmt_arg<3, fmt_shifter>;
using arg4_shifter = fmt_arg<4, fmt_shifter>;

// declare reg_indir for arg1+
using arg1_reg_indir = fmt_arg<1, fmt_reg_indir>;
using arg2_reg_indir = fmt_arg<2, fmt_reg_indir>;
using arg3_reg_indir = fmt_arg<3, fmt_reg_indir>;

// declare register-set for arg1, arg2
using arg1_regset  = fmt_arg<1, gen_0b16>;
using arg2_regset  = fmt_arg<2, gen_0b16>;

//
// Declare types used in INSN definitions
//

// conventional name for format used by `OPC_LIST`
struct FMT_LIST : fmt_list, arg1_12b4 {};

// conventional name for `no-args` formatter
struct FMT_X     : fmt_gen {};

// conventional name for `not-yet-implemented` formatter
struct FMT_      : fmt_gen {};

// Branches:
// insert 24-bit offset. linker handles out-of-range case
struct FMT_B : fmt_branch, fmt_arg<1, fmt_branch24> {};

// `bx` instruction needs to drop a ARM_V4BX reloc to help linker
struct FMT_BX : fmt_bx, arg1_00b4 {};

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
struct FMT_12_16_AS  : FMT_12_16, arg3_addsub       {};

// addressing mode 1: no Rn (eg mov)
struct FMT_12_0   : fmt_gen, arg1_12b4, arg2_00b4     {};
struct FMT_12_0_S : FMT_12_0, arg3_shifter            {};
struct FMT_12_F   : fmt_gen, arg1_12b4, arg2_fixed    {};

// addressing mode 1: no Rd (eg cmp)`
struct FMT_16_0   : fmt_gen, arg1_16b4, arg2_00b4  {};
struct FMT_16_0_S : FMT_16_0, arg3_shifter         {};
struct FMT_16_F   : fmt_gen, arg1_16b4, arg2_fixed {};

// support ARM5 addressing mode 2 (aka register indirect)
struct FMT_LD  : fmt_gen, arg1_12b4, arg2_reg_indir {};
struct FMT_PLD : fmt_gen, arg1_reg_indir {};

// `srs`: 5 LSBs are immed value
struct FMT_X_I5: fmt_gen, fmt_arg<2, fmt32_generic<0, 5>> {};

// addressing mode 4
struct FMT_RS    : fmt_gen, arg1_regset            {};
struct FMT_16_RS : fmt_gen, arg1_16b4, arg2_regset {};

struct FMT_8_12_M4 : fmt_gen, arg1_08b4, arg2_12b4, arg3_reg_indir {};
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
struct FMT_BKPT    : FMT_I24 {};
}
#endif




