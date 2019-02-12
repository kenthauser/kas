#ifndef KAS_Z80_Z80_FORMATS_DEFN_H
#define KAS_Z80_Z80_FORMATS_DEFN_H

// 1. Remove `index` infrastructure
// 2. Split into virtual functions, "workers" and combiners
// 3. Add in `opc&` stuff

//#include "z80_formats_impl.h"
#include "target/tgt_format.h"
//#include "z80_formats_opc.h"

namespace kas::z80::opc
{
// use generic mixin
template <unsigned...Ts>
using fmt_generic = tgt::opc::tgt_fmt_generic<z80_mcode_t, Ts...>;

template <unsigned N, typename T>
using fmt_arg = tgt::opc::tgt_fmt_arg<z80_mcode_t, N, T>;

// declare mixin classes to override virtual functions
// override insert/extract pairs together...

// extract from first word
using gen_0b3 = fmt_generic<0, 3>;
using gen_3b3 = fmt_generic<3, 3>;
using gen_3b2 = fmt_generic<3, 2>;

using gen_4b2 = fmt_generic<4, 2>;
using gen_4b1 = fmt_generic<4, 1>;

// extract from second word
using gen_1w0b3 = fmt_generic<0, 3, 1>;
using gen_1w3b3 = fmt_generic<3, 3, 1>;
using gen_1w3b2 = fmt_generic<3, 2, 1>;

using gen_1w4b2 = fmt_generic<4, 2, 1>;

// Z80 lists DST first
using arg1_0b3 = fmt_arg<1, gen_0b3>;
using arg1_3b3 = fmt_arg<1, gen_3b3>;
using arg2_0b3 = fmt_arg<2, gen_0b3>;
using arg2_3b3 = fmt_arg<2, gen_3b3>;

using arg1_3b2 = fmt_arg<1, gen_1w3b2>;
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

// get `opc` generic base classes
using fmt_gen  = tgt::opc::tgt_fmt_opc_gen <z80_mcode_t>;
using fmt_list = tgt::opc::tgt_fmt_opc_list<z80_mcode_t>;

// used by OPC_LIST instructions
struct FMT_LIST     : fmt_list, arg1_3b3, arg2_0b3 {};

// general registers are 3-bits shifted 0 or 3 bits
struct FMT_X       : fmt_gen {};
struct FMT_0       : fmt_gen, arg1_0b3 {};
struct FMT_X_0     : fmt_gen, arg2_0b3 {};
struct FMT_3_0     : fmt_gen, arg1_3b3, arg2_0b3 {};
struct FMT_3       : fmt_gen, arg1_3b3 {};
struct FMT_X_3     : fmt_gen, arg2_3b3 {};

// IM: 2 bits modified & shifed 3 bits
struct FMT_1W3B2   : fmt_gen, arg1_1w3b2 {};

// DBL registers are two bits shifted 4
struct FMT_4       : fmt_gen, arg1_4b2 {};
struct FMT_X_4     : fmt_gen, arg2_4b2 {};

// BC/DE instructions: 1 bit shifted 4
struct FMT_4B1     : fmt_gen, arg1_4b1 {};
struct FMT_X_4B1   : fmt_gen, arg2_4b1 {};

// JR uses special OPCODE to generate machine code
struct FMT_JR      : fmt_gen {};
struct FMT_JR_3    : fmt_gen, arg1_3b2 {};
struct FMT_DJNZ    : fmt_gen {};

// CB instructions
struct FMT_1W4     : fmt_gen, arg1_1w4b2 {};
struct FMT_X_1W4   : fmt_gen, arg2_1w4b2 {};
struct FMT_1W0     : fmt_gen, arg1_1w0b3 {};
struct FMT_1W3     : fmt_gen, arg1_1w3b3 {};
struct FMT_X_1W3   : fmt_gen, arg2_1w3b3 {};
struct FMT_1W3_1W0 : fmt_gen, arg1_1w3b3, arg2_1w0b3 {};


}
#endif


