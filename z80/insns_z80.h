#ifndef KAS_Z80_M68000_DEFNS_H
#define KAS_Z80_M68000_DEFNS_H
//
// Define the Z80 instructions. 
//
// For information in the table format, see `target/tgt_defn_trait.h`
//
// The format names (and naming convention) are in `z80_opcode_formats.h`
//
// The argument validators are in `z80_arg_validate_arg.h`
 //
// The common validors with ambiguous names are:
//
// REG          : allow 8-bit registers: A, B, C, D, E, H, L
// REG_GEN      : allow REG and (HL), (IX+n), (IY+n)
// REG_<name>   : allow only register with <name>

// REG_DBL_SP   : allow BC, DE, HL, SP, IX, IY
// REG_DBL_AF   : allow BC, DE, HL, AF, IX, IY
// REG_IDX      : allow HL, IX, IY

// IMMED_8      :  8-bit immed arg
// IMMED_16     : 16-bit immed arg

// INDIR        : indirect memory address
// INDIR_8      : indirect I/O address (8-bits)
// INDIR_BC_DE  : allow only (BC), (DE)
// INDIR_SP     : allow only (SP)
// INDIR_IDX    : allow (HL), (IX), (IY)

// CC           : condition codes for JP, CALL, & RET
// JR_CC        : condition codes for JR
//
// Conventions is for formater type names to list "shifts" for args in order. 
// shift of `X` indicates arg is not inserted in machine code
// for shift if `X`, arg is either determined by "validator" (eg REG_A) or immediate
// immediate arg validators can determine emit size


#include "z80_insn_common.h"

namespace kas::z80::opc::gen
{
//using namespace common;

#define STR KAS_STRING


using z80_insn_ld_l = list<list<>
//
// Dummy machine-code for "list" opcode
//

, defn<sz_w, STR("*LIST*"), OP<0>, FMT_LIST, REG_GEN, REG_GEN>

//
// 8-bit load group
//

, defn<sz_b, STR("ld"), OP<0x40>, FMT_3_0, REG    , REG_GEN>
, defn<sz_b, STR("ld"), OP<0x40>, FMT_3_0, REG_GEN, REG>
, defn<sz_b, STR("ld"), OP<0x06>, FMT_3  , REG_GEN, IMMED_8>

, defn<sz_b, STR("ld"), OP<0x0a>, FMT_X_4B1, REG_A, INDIR_BC_DE>
, defn<sz_b, STR("ld"), OP<0x02>, FMT_4B1  , INDIR_BC_DE, REG_A>

, defn<sz_b, STR("ld"), OP<0x3a>, FMT_X  , REG_A, INDIR>
, defn<sz_b, STR("ld"), OP<0x32>, FMT_X  , INDIR, REG_A>

, defn<sz_b, STR("ld"), OP<0xed57>, FMT_X, REG_A, REG_I>
, defn<sz_b, STR("ld"), OP<0xed5f>, FMT_X, REG_A, REG_R>
, defn<sz_b, STR("ld"), OP<0xed47>, FMT_X, REG_I, REG_A>
, defn<sz_b, STR("ld"), OP<0xed4f>, FMT_X, REG_R, REG_A>

//
// 16-bit load group
//

// load Immed
, defn<sz_w, STR("ld"), OP<0x01>  , FMT_4, REG_DBL_SP, IMMED_16>

// load from memory
, defn<sz_w, STR("ld"), OP<0x2a>  , FMT_X  , REG_IDX   , INDIR>
, defn<sz_w, STR("ld"), OP<0xed4b>, FMT_1W4, REG_DBL_SP, INDIR>

// save to memory
, defn<sz_w, STR("ld"), OP<0x22>  , FMT_X    , INDIR, REG_IDX>
, defn<sz_w, STR("ld"), OP<0xed43>, FMT_X_1W4, INDIR, REG_DBL_SP>

// load SP from HL, IX, IY
, defn<sz_w, STR("ld"), OP<0xf9>  , FMT_X, REG_SP, REG_IDX>

// push/pop BC, DE, HL, AF, IX, IY
, defn<sz_w, STR("push"), OP<0xc5>, FMT_4, REG_DBL_AF>
, defn<sz_w, STR("pop") , OP<0xc1>, FMT_4, REG_DBL_AF>

// exchange, block transfer, search group

, defn<sz_v, STR("ex")  , OP<0xeb>, FMT_X, REG_DE, REG_HL>
, defn<sz_v, STR("ex")  , OP<0xe3>, FMT_X, INDIR_SP, REG_IDX>
, defn<sz_v, STR("ex")  , OP<0x08>, FMT_X, REG_AF, REG_AF>
, defn<sz_v, STR("exaf"), OP<0x08>>
, defn<sz_v, STR("exx") , OP<0xd9>>

, defn<sz_v, STR("ldi") , OP<0xeda0>>
, defn<sz_v, STR("ldir"), OP<0xedb0>>
, defn<sz_v, STR("ldd") , OP<0xeda8>>
, defn<sz_v, STR("lddr"), OP<0xedb8>>
, defn<sz_v, STR("cpi") , OP<0xeda1>>
, defn<sz_v, STR("cpir"), OP<0xedb1>>
, defn<sz_v, STR("cpd") , OP<0xeda9>>
, defn<sz_v, STR("cpdr"), OP<0xedb9>>
>;


// math metafunction: generate accumulator formats
// 1) add a,b   ( (hl), (idx+n) allowed )
// 2) add b     (a implied)
// 5) add a,#4
// 6) add #4    (a implied)

template <typename NAME, uint32_t OPC, uint32_t OPC_IMMED>
using math = list<list<>
    , defn<sz_b, NAME, OP<OPC>      , FMT_X_0  , REG_A, REG_GEN>
    , defn<sz_b, NAME, OP<OPC>      , FMT_0    , REG_GEN>
    , defn<sz_b, NAME, OP<OPC_IMMED>, FMT_X    , REG_A, IMMED_8>
    , defn<sz_b, NAME, OP<OPC_IMMED>, FMT_X    , IMMED_8>
    >;

using z80_insn_math_l = list<list<>
//
// 8-bit arithmetic group
//
, math<STR("add"), 0x80, 0xc6>      // args: reg_a code, immed code
, math<STR("adc"), 0x88, 0xce>
, math<STR("sub"), 0x90, 0xd6>
, math<STR("sbc"), 0x98, 0xde>
, math<STR("and"), 0xa0, 0xe6>
, math<STR("xor"), 0xa8, 0xee>
, math<STR("or") , 0xb0, 0xf6>
, math<STR("cp") , 0xb8, 0xfe>

, defn<sz_b, STR("inc"), OP<0x04>, FMT_3, REG_GEN>
, defn<sz_b, STR("dec"), OP<0x05>, FMT_3, REG_GEN>


//
// General purpose Arithmetic & CPU control groups
//

, defn<sz_b, STR("daa") , OP<0x27>>
, defn<sz_b, STR("cpl") , OP<0x2f>>
, defn<sz_b, STR("neg") , OP<0xed44>>
, defn<sz_b, STR("ccf") , OP<0x3f>>
, defn<sz_b, STR("scf") , OP<0x37>>
, defn<sz_b, STR("nop") , OP<0x00>>
, defn<sz_b, STR("halt"), OP<0x76>>
, defn<sz_b, STR("di")  , OP<0xf3>>
, defn<sz_b, STR("ei")  , OP<0xfb>>
, defn<sz_b, STR("im")  , OP<0xed46>, FMT_1W3B2, IMMED_IM>

//
// 16-bit arithmetic group
//

// NB: only self add of dbl accumuators allowed. ie no add ix, hl
// validators enforce by using single value of `arg::prefix`
, defn<sz_w, STR("add"), OP<0x09>  , FMT_X_4  , REG_IDX, REG_DBL_SP>
, defn<sz_w, STR("adc"), OP<0xed4a>, FMT_X_1W4, REG_IDX, REG_DBL_SP>
, defn<sz_w, STR("sbc"), OP<0xed42>, FMT_X_1W4, REG_IDX, REG_DBL_SP>

, defn<sz_w, STR("inc"), OP<0x03>, FMT_4, REG_DBL_SP>
, defn<sz_w, STR("dec"), OP<0x0b>, FMT_4, REG_DBL_SP>

// 
// Rotate & shift group
//

// 8080 instructions
, defn<sz_b, STR("rlca"), OP<0x07>>
, defn<sz_b, STR("rrca"), OP<0x0f>>
, defn<sz_b, STR("rla") , OP<0x17>>
, defn<sz_b, STR("rra") , OP<0x1f>>

// Z80 shifts
, defn<sz_b, STR("rlc") , OP<0xcb00>, FMT_1W0, REG_GEN>
, defn<sz_b, STR("rrc") , OP<0xcb08>, FMT_1W0, REG_GEN>
, defn<sz_b, STR("rl")  , OP<0xcb10>, FMT_1W0, REG_GEN>
, defn<sz_b, STR("rr")  , OP<0xcb18>, FMT_1W0, REG_GEN>
, defn<sz_b, STR("sla") , OP<0xcb20>, FMT_1W0, REG_GEN>
, defn<sz_b, STR("sra") , OP<0xcb28>, FMT_1W0, REG_GEN>
, defn<sz_b, STR("srl") , OP<0xcb38>, FMT_1W0, REG_GEN>

, defn<sz_b, STR("rld") , OP<0xed6f>>
, defn<sz_b, STR("rrd") , OP<0xed67>>

//
// Bit set, reset, and test group
//

, defn<sz_b, STR("bit"), OP<0xcb40>, FMT_1W3_1W0, BIT_NUM, REG_GEN>
, defn<sz_b, STR("res"), OP<0xcb80>, FMT_1W3_1W0, BIT_NUM, REG_GEN>
, defn<sz_b, STR("set"), OP<0xcbc0>, FMT_1W3_1W0, BIT_NUM, REG_GEN>
>;

using z80_insn_jmp_l = list<list<>
//
// Jump group
//
#
, defn<sz_v, STR("jp")  , OP<0xc3>, FMT_X   , DIRECT>
, defn<sz_v, STR("jp")  , OP<0xc2>, FMT_3   , CC, DIRECT>
, defn<sz_v, STR("jp")  , OP<0xe9>, FMT_X   , INDIR_IDX>
, defn<sz_v, STR("jr")  , OP<0x18>, FMT_JR  , DIRECT>
, defn<sz_v, STR("jr")  , OP<0x20>, FMT_JRCC, JR_CC, DIRECT>
, defn<sz_v, STR("djnz"), OP<0x10>, FMT_DJNZ, DIRECT>

//
// Call and Return group
//

, defn<sz_v, STR("call"), OP<0xcd>, FMT_X, DIRECT>
, defn<sz_v, STR("call"), OP<0xc4>, FMT_3, CC, DIRECT>
, defn<sz_v, STR("ret") , OP<0xc9>>
, defn<sz_v, STR("ret") , OP<0xc0>, FMT_3, CC>

, defn<sz_v, STR("rst") , OP<0xc7>, FMT_3, IMMED_RST>
, defn<sz_v, STR("reti"), OP<0xed4d>>
, defn<sz_v, STR("retn"), OP<0xed45>>
>;

using z80_insn_io_l = list<list<>
//
// Input and Output group
//

, defn<sz_b, STR("in")  , OP<0xdb>  , FMT_X    , INDIR_8>
, defn<sz_b, STR("in")  , OP<0xdb>  , FMT_X    , REG_A  , INDIR_8>
, defn<sz_b, STR("in")  , OP<0xed40>, FMT_1W3  , REG    , INDIR_C>
, defn<sz_b, STR("out") , OP<0xd3>  , FMT_X    , INDIR_8>
, defn<sz_b, STR("out") , OP<0xd3>  , FMT_X    , INDIR_8, REG_A>
, defn<sz_b, STR("out") , OP<0xed41>, FMT_X_1W3, INDIR_C, REG>

, defn<sz_b, STR("ini") , OP<0xeda2>>
, defn<sz_b, STR("inir"), OP<0xedb2>>
, defn<sz_b, STR("ind") , OP<0xedaa>>
, defn<sz_b, STR("indr"), OP<0xedba>>
, defn<sz_b, STR("outi"), OP<0xeda3>>
, defn<sz_b, STR("otir"), OP<0xedb3>>
, defn<sz_b, STR("outd"), OP<0xedab>>
, defn<sz_b, STR("otdr"), OP<0xedbb>>
>;

using z80_insn_list = list<list<>
                         , z80_insn_ld_l
                         , z80_insn_math_l
                         , z80_insn_jmp_l
                         , z80_insn_io_l
                         >;
}

namespace kas::z80::opc
{
    template <> struct z80_insn_defn_list<OP_Z80_GEN> : gen::z80_insn_list {};
}

#undef STR

#endif

