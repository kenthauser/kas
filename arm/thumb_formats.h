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
using arg3_6I5     = fmt_arg<3, gen_06b5>;  // 5 bits of shift 

//
// Declare types used in INSN definitions
//

// conventional name for `not-yet-implemented` formatter
struct FMT_         : fmt_gen {};

// conventional name for `no-args` formatter (aka generic base type)
struct FMT_X        : fmt_gen {};

// General Instructions
struct FMT_0_3      : fmt_gen, arg1_00b3, arg2_03b3 {};
struct FMT_0_3_6    : FMT_0_3, arg3_06b3 {};
struct FMT_0_3_ABS5 : FMT_0_3, arg3_6I5 {};

struct FMT_8_I8     : fmt_gen, arg1_08b3, arg2_I8 {};
struct FMT_8_X_I8   : fmt_gen, arg1_08b3, arg3_I8 {};

// R* support relocations
struct FMT_8_R8     : fmt_gen, arg1_08b3, arg2_I8 {};
struct FMT_8_X_R8   : fmt_gen, arg1_08b3, arg3_I8 {};

struct FMT_8_PC8    : fmt_gen, arg1_08b3, fmt_arg<2, fmt_pc8> {};
struct FMT_8_X_PC8  : fmt_gen, arg1_08b3, fmt_arg<3, fmt_pc8> {};

struct FMT_X_I7     : fmt_gen, fmt_arg<2, gen_00b7> {};

// high register format
struct FMT_7H0_6H3  : fmt_gen, fmt_arg<1, fmt_regh<7, 0>>
                             , fmt_arg<2, fmt_regh<6, 3>> {};

// load/store formats
struct FMT_0_6X3    : fmt_gen, arg1_00b3
                             , fmt_arg<2, fmt16_generic<3, 6>> {};
struct FMT_0_8X3    : fmt_gen, arg1_00b3
                             , fmt_arg<2, fmt16_generic<3, 8>> {};
struct FMT_0_ABS5   : fmt_gen, arg1_00b3
                             , fmt_arg<2, fmt_indir5> {};
// special for PUSH/POP
struct FMT_I9       : fmt_gen, fmt_arg<1, fmt16_generic<0, 9>> {};

// special for exception ops
struct FMT_I8       : fmt_gen, fmt_arg<1, fmt16_generic<0, 8>> {};

// Branches:
struct FMT_TJU      : fmt_branch {}; //, fmt_arg<1, fmt_jump11> {};
struct FMT_TJC      : fmt_branch {}; // , fmt_arg<1, fmt_jump8> {};
struct FMT_TC       : fmt_branch, fmt_arg<1, fmt_thb_branch24<ARM_G1>> {};
struct FMT_TJW      : fmt_branch, fmt_arg<1, fmt_thb_branch24<ARM_G2>> {};

// BX/BLX: 4-bit register, shifted 3
struct FMT_3I4      : fmt_gen, fmt_arg<1, fmt16_generic<3, 4>> {};

// single arg formats
struct FMT_0        : fmt_gen, arg1_00b3 {};
}
#endif




