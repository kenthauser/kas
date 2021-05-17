#ifndef KAS_Z80_Z80_FORMATS_H
#define KAS_Z80_Z80_FORMATS_H

// Declare Z80 argument inserters.
 
// Generate the "formatters" used to insert/extract arguments
// into the `machine-code` binary.
//
// The `formatters` are named as follows:
//
// 1. The name "base" (usually FMT_) indicates the "machine_code" virtual type
//    to use. (these are named in `formats_opc`)
//
// 2. Following the "base", underscores are used to separate inserter for each
//    argument. Thus `FMT_3_0` inserts three arguments, coded `3` and `0`
// 
// 3. By convention, the inserter `X` is used when arg is validated but not
//    inserted into machine code
//
// 4. Most args are 3-bits. These are encoded with either `0` or `3`.
//    Some DBL register opcode use a 2-bit code shifted 4. These are encoded `4`
//    Special opcodes use the xWx format described in `tgt_format.h`
//
 

#include "z80_formats_opc.h"       // generic target definitions

namespace kas::z80::opc
{
/// declare mixin classes to override virtual functions
// override insert/extract pairs together...
// NB: fmt_generic args are: shift, width, word = 0

// insert/extract from first word
using gen_0b3 = fmt_generic<0, 3>;
using gen_3b3 = fmt_generic<3, 3>;
using gen_3b2 = fmt_generic<3, 2>;

using gen_4b2 = fmt_generic<4, 2>;
using gen_4b1 = fmt_generic<4, 1>;

// insert/extract from second word
using gen_1w0b3 = fmt_generic<0, 3, 1>;
using gen_1w3b3 = fmt_generic<3, 3, 1>;
using gen_1w3b2 = fmt_generic<3, 2, 1>;
using gen_1w4b2 = fmt_generic<4, 2, 1>;

// bind `inserters` with `arg num`
using arg1_0b3 = fmt_arg<1, gen_0b3>;
using arg1_3b3 = fmt_arg<1, gen_3b3>;
using arg2_0b3 = fmt_arg<2, gen_0b3>;
using arg2_3b3 = fmt_arg<2, gen_3b3>;

using arg1_3b2 = fmt_arg<1, gen_3b2>;
using arg2_3b2 = fmt_arg<2, gen_3b2>;

using arg1_4b2  = fmt_arg<1, gen_4b2>;
using arg2_4b2  = fmt_arg<2, gen_4b2>;

using arg1_4b1  = fmt_arg<1, gen_4b1>;
using arg2_4b1  = fmt_arg<2, gen_4b1>;

// second word args
using arg1_1w0b3  = fmt_arg<1, gen_1w0b3>;
using arg1_1w3b3  = fmt_arg<1, gen_1w3b3>;
using arg2_1w0b3  = fmt_arg<2, gen_1w0b3>;
using arg2_1w3b3  = fmt_arg<2, gen_1w3b3>;

using arg1_1w3b2  = fmt_arg<1, gen_1w3b2>;
using arg2_1w3b2  = fmt_arg<2, gen_1w3b2>;

using arg1_1w4b2  = fmt_arg<1, gen_1w4b2>;
using arg2_1w4b2  = fmt_arg<2, gen_1w4b2>;

//
// Declare types used in INSN definitions
//

// conventional name for format used by `OPC_LIST`
// NB: VALIDATORs are REG_GEN, REG_GEN
// NB: purposefully make unique (arg order doesn't match move)
struct FMT_LIST     : fmt_list, arg1_0b3, arg2_3b3 {};
 
// conventional name for `no-args` formatter
struct FMT_X       : fmt_gen {};
 
struct FMT_BRANCH  : fmt_branch {};
struct FMT_DJNZ    : fmt_djnz         {};
 
///////////////////////////////////////////////////////////////////////////

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
}

#endif


