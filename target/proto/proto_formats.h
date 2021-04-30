#ifndef KAS_PROTO_UC_PROTO_UC_FORMATS_H
#define KAS_PROTO_UC_PROTO_UC_FORMATS_H

// Generate the "formatters" used to insert/extract arguments
// into the `machine-code` binary.
//
// NAMING CONVENTIONS ARE FOR CONVENIENCE ONLY, AND ARE NOT ENFORCED
//
// Conventionally `formatters` are named as follows:
//
// 1. The name "base" (usually FMT_) indicates the "machine_code" virtual type
//    to use. (these are named in `formats_opc`, et.al.)
//
// 2. Following the "base", underscores are used to separate inserter for
//    each argument.
//    Thus `FMT_a1_a2_a3` inserts three arguments, coded `a1`, `a2`, and `a3`.
// 
// 3. By convention, the inserter `X` is used when arg is validated but not
//    inserted into machine code
//
// 4. For the PROTO_UC, most insertions are 3-bits register numbers.
//    For register args, the convention is to use 0-15 to represent shifts
//    in the first word, and 16-31 to represent shifts in the second word.
//

#include "PROTO_LC_formats_opc.h"       // generic target definitions

namespace kas::PROTO_LC::opc
{
// EXAMPLES:

// declare mixin classes to override virtual functions
// override insert/extract pairs together...

// associate format function with source/dest argument
// 3-bit reg# stored in four places
template <int SHIFT, int WORD = 0>
using fmt_reg = fmt_generic<SHIFT, 3, WORD>;

// argX shifted zero bits
using arg1_0      = fmt_arg<1, fmt_reg<0>>;
using arg2_0      = fmt_arg<2, fmt_reg<0>>;

// argX shifted three bits
using arg1_3      = fmt_arg<1, fmt_reg<3>>;
using arg2_3      = fmt_arg<2, fmt_reg<3>>;     // special for `LIST`

///////////////////////////////////////////////////////////////////////////


// used by OPC_LIST instructions
struct FMT_LIST     : fmt_list, arg1_0, arg2_3 {};


// branch formats have implied argument format
struct FMT_BRANCH   : fmt_branch {};

// name template: FMT_ <First Arg (src)> _ <Second Arg (dst)>
struct FMT_X        : fmt_gen {};

struct FMT_3_0      : fmt_gen, arg1_3, arg2_0 {};
}
#endif


