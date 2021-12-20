#ifndef KAS_ARM_THUMB_FORMATS_H
#define KAS_ARM_THUMB_FORMATS_H

// Declare ARM argument inserters.
// See `target/tgt_format.h` for naming convention.

#include "arm_formats_opc.h"
#include "thumb_formats_ops.h"

namespace kas::arm::opc::thumb
{
// declare mixin classes to override virtual functions
// override insert/extract pairs together...
// NB: fmt_generic args are: shift, width, word = 0

// insert/extract 8-bit value from first word
using gen_00b3 = fmt16_generic<0, 3>;
using gen_03b3 = fmt16_generic<3, 3>;
using gen_06b3 = fmt16_generic<6, 3>;
using gen_08b3 = fmt16_generic<8, 3>;

using gen_06b5 = fmt16_generic<6, 5>;   // shift count

using gen_00b7 = fmt16_generic<0, 7>;
using gen_00b8 = fmt16_generic<0, 8>;

// register/value inserters: declare 4-bit locations for several args
using arg1_00b3 = fmt_arg<1, gen_00b3>;
using arg1_08b3 = fmt_arg<1, gen_08b3>;
using arg2_03b3 = fmt_arg<2, gen_03b3>;
using arg3_06b3 = fmt_arg<3, gen_06b3>;


// declare immed for arg 2+
using arg2_I8      = fmt_arg<2, gen_00b8>;
using arg3_I8      = fmt_arg<3, gen_00b8>;
using arg3_I7      = fmt_arg<3, gen_00b7>;
using arg3_6I5     = fmt_arg<3, gen_06b5>;  // 5 bits of shift 

// declare reg_indir for arg1+
using arg1_reg_indir = fmt_arg<1, fmt_reg_indir>;
using arg2_reg_indir = fmt_arg<2, fmt_reg_indir>;
using arg3_reg_indir = fmt_arg<3, fmt_reg_indir>;


//
// Declare types used in INSN definitions
//

// conventional name for `not-yet-implemented` formatter
struct FMT_         : fmt_gen {};

// conventional name for `no-args` formatter (aka generic base type)
struct FMT_X        : fmt_gen {};

// General Instructions
struct FMT_0_3      : FMT_X  , arg1_00b3, arg2_03b3 {};
struct FMT_0_3_6    : FMT_0_3, arg3_06b3 {};
struct FMT_0_3_ABS5 : FMT_0_3, arg3_6I5 {};

struct FMT_8_I8     : FMT_X, arg1_08b3, arg2_I8 {};
struct FMT_8_X_I8   : FMT_X, arg1_08b3, arg3_I8 {};

struct FMT_8_PC8    : FMT_X, arg1_08b3, fmt_arg<2, fmt_pc8> {};
struct FMT_8_X_PC8  : FMT_X, arg1_08b3, fmt_arg<3, fmt_pc8> {};

struct FMT_X_X_I7   : FMT_X, arg3_I7 {};

// high register format
struct FMT_7H0_6H3  : FMT_X, fmt_arg<1, fmt_regh<7, 0>>
                           , fmt_arg<2, fmt_regh<6, 3>> {};

// load/store formats
struct FMT_0_6X3    : FMT_X, arg1_00b3
                           , fmt_arg<2, fmt16_generic<3, 6>> {};
struct FMT_0_8X3    : FMT_X, arg1_00b3
                           , fmt_arg<2, fmt16_generic<3, 8>> {};
struct FMT_0_INDIR5 : FMT_X, arg1_00b3
                           , fmt_arg<2, fmt_indir5> {};
// special for PUSH/POP
struct FMT_I9       : FMT_X, fmt_arg<1, fmt16_generic<0, 9>> {};

// Branches:
// insert 24-bit offset. linker handles out-of-range case
struct FMT_B : fmt_branch, fmt_arg<1, fmt_branch24> {};

struct FMT_J11      : FMT_X, fmt_arg<1, fmt_jump11> {};
struct FMT_J8       : FMT_X, fmt_arg<1, fmt_jump8> {};
struct FMT_CALL22   : FMT_X, fmt_arg<1, fmt_call22> {};

// BX/BLX: 4-bit register, shifted 3
struct FMT_3I4  : FMT_X, fmt_arg<1, fmt16_generic<3, 4>> {};

// `bx` instruction needs to drop a ARM_V4BX reloc to help linker
struct FMT_BX : fmt_bx, arg1_00b3 {};

// single arg formats
struct FMT_0     : fmt_gen, arg1_00b3 {};
}
#endif




