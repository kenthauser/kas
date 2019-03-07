#ifndef KAS_Z80_M68000_DEFNS_H
#define KAS_Z80_M68000_DEFNS_H

// While the tables look cryptic, the MPL fields are generally as follows:
//
// 1. opcode name. 
//
// 2. base machine code. 8-bit or 16-bit value
//
// 3. opcode formater: how to insert args into machine code
//
// 4+ validator(s) for z80 arguments, if any. Maximum 2.
//
// The format names (and naming convention) are in `z80_opcode_formats.h`
// The argument validators are in `z80_arg_validate.h`

 
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

, defn<STR("*LIST*"), 0, FMT_LIST, REG_GEN, REG_GEN>

//
// 8-bit load group
//

, defn<STR("ld"), 0x40, FMT_3_0, REG    , REG_GEN>
#if 1
, defn<STR("ld"), 0x40, FMT_3_0, REG_GEN, REG>
, defn<STR("ld"), 0x06, FMT_3  , REG_GEN, IMMED_8>

, defn<STR("ld"), 0x0a, FMT_X_4B1, REG_A, INDIR_BC_DE>
, defn<STR("ld"), 0x02, FMT_4B1  , INDIR_BC_DE, REG_A>

, defn<STR("ld"), 0x3a, FMT_X  , REG_A, INDIR>
, defn<STR("ld"), 0x32, FMT_X  , INDIR, REG_A>

, defn<STR("ld"), 0xed57, FMT_X, REG_A, REG_I>
, defn<STR("ld"), 0xed5f, FMT_X, REG_A, REG_R>
, defn<STR("ld"), 0xed47, FMT_X, REG_I, REG_A>
, defn<STR("ld"), 0xed4f, FMT_X, REG_R, REG_A>

//
// 16-bit load group
//

// load Immed
, defn<STR("ld"), 0x01  , FMT_4, REG_DBL_SP, IMMED_16>

// load from memory
, defn<STR("ld"), 0x2a  , FMT_X  , REG_IDX   , INDIR>
, defn<STR("ld"), 0xed4b, FMT_1W4, REG_DBL_SP, INDIR>

// save to memory
, defn<STR("ld"), 0x22  , FMT_X    , INDIR, REG_IDX>
, defn<STR("ld"), 0xed43, FMT_X_1W4, INDIR, REG_DBL_SP>

// load SP from HL, IX, IY
, defn<STR("ld"), 0xf9, FMT_X, REG_SP, REG_IDX>

// push/pop BC, DE, HL, AF, IX, IY
, defn<STR("push"), 0xc5, FMT_4, REG_DBL_AF>
, defn<STR("pop") , 0xc1, FMT_4, REG_DBL_AF>

// exchange, block transfer, search group

, defn<STR("ex")  , 0xeb, FMT_X, REG_DE, REG_HL>
, defn<STR("ex")  , 0xe3, FMT_X, INDIR_SP, REG_IDX>
, defn<STR("ex")  , 0x08, FMT_X, REG_AF, REG_AF>
, defn<STR("exaf"), 0x08>
, defn<STR("exx") , 0xd9>

, defn<STR("ldi") , 0xeda0>
, defn<STR("ldir"), 0xedb0>
, defn<STR("ldd") , 0xeda8>
, defn<STR("lddr"), 0xedb8>
, defn<STR("cpi") , 0xeda1>
, defn<STR("cpir"), 0xedb1>
, defn<STR("cpd") , 0xeda9>
, defn<STR("cpdr"), 0xedb9>
#endif
>;


// math metafunction: generate accumulator formats
// 1) add a,b   ( (hl), (idx+n) allowed )
// 2) add b     (a implied)
// 5) add a,#4
// 6) add #4    (a implied)

template <typename NAME, uint32_t OPC, uint32_t OPC_IMMED>
using math = list<list<>
    , defn<NAME, OPC      , FMT_X_0  , REG_A, REG_GEN>
    , defn<NAME, OPC      , FMT_0    , REG_GEN>
    , defn<NAME, OPC_IMMED, FMT_X    , REG_A, IMMED_8>
    , defn<NAME, OPC_IMMED, FMT_X    , IMMED_8>
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

, defn<STR("inc"), 0x04, FMT_3, REG_GEN>
, defn<STR("dec"), 0x05, FMT_3, REG_GEN>


//
// General purpose Arithmetic & CPU control groups
//

, defn<STR("daa") , 0x27>
, defn<STR("cpl") , 0x2f>
, defn<STR("neg") , 0xed44>
, defn<STR("ccf") , 0x3f>
, defn<STR("scf") , 0x37>
, defn<STR("nop") , 0x00>
, defn<STR("halt"), 0x76>
, defn<STR("di")  , 0xf3>
, defn<STR("ei")  , 0xfb>
, defn<STR("im")  , 0xed46, FMT_1W3B2, IMMED_IM>

//
// 16-bit arithmetic group
//

// NB: only self add of dbl accumuators allowed. ie no add ix, hl
// validators enforce by using single value of `arg::prefix`
, defn<STR("add"), 0x09  , FMT_X_4  , REG_IDX, REG_DBL_SP>
, defn<STR("adc"), 0xed4a, FMT_X_1W4, REG_IDX, REG_DBL_SP>
, defn<STR("sbc"), 0xed42, FMT_X_1W4, REG_IDX, REG_DBL_SP>

, defn<STR("inc"), 0x03, FMT_4, REG_DBL_SP>
, defn<STR("dec"), 0x0b, FMT_4, REG_DBL_SP>

// 
// Rotate & shift group
//

// 8080 instructions
, defn<STR("rlca"), 0x07>
, defn<STR("rrca"), 0x0f>
, defn<STR("rla") , 0x17>
, defn<STR("rra") , 0x1f>

// Z80 shifts
, defn<STR("rlc") , 0xcb00, FMT_1W0, REG_GEN>
, defn<STR("rrc") , 0xcb08, FMT_1W0, REG_GEN>
, defn<STR("rl")  , 0xcb10, FMT_1W0, REG_GEN>
, defn<STR("rr")  , 0xcb18, FMT_1W0, REG_GEN>
, defn<STR("sla") , 0xcb20, FMT_1W0, REG_GEN>
, defn<STR("sra") , 0xcb28, FMT_1W0, REG_GEN>
, defn<STR("srl") , 0xcb38, FMT_1W0, REG_GEN>

, defn<STR("rld") , 0xed6f>
, defn<STR("rrd") , 0xed67>

//
// Bit set, reset, and test group
//

, defn<STR("bit"), 0xcb40, FMT_1W3_1W0, BIT_NUM, REG_GEN>
, defn<STR("res"), 0xcb80, FMT_1W3_1W0, BIT_NUM, REG_GEN>
, defn<STR("set"), 0xcbc0, FMT_1W3_1W0, BIT_NUM, REG_GEN>
>;

using z80_insn_jmp_l = list<list<>
//
// Jump group
//
#
, defn<STR("jp")  , 0xc3, FMT_X   , DIRECT>
, defn<STR("jp")  , 0xc2, FMT_3   , CC, DIRECT>
, defn<STR("jp")  , 0xe9, FMT_X   , INDIR_IDX>
, defn<STR("jr")  , 0x18, FMT_JR  , DIRECT>
, defn<STR("jr")  , 0x20, FMT_JR_3, JR_CC, DIRECT>
, defn<STR("djnz"), 0x10, FMT_DJNZ, DIRECT>

//
// Call and Return group
//

, defn<STR("call"), 0xcd, FMT_X, DIRECT>
, defn<STR("call"), 0xc4, FMT_3, CC, DIRECT>
, defn<STR("ret") , 0xc9>
, defn<STR("ret") , 0xc0, FMT_3, CC>

, defn<STR("rst") , 0xc7, FMT_3, IMMED_RST>
, defn<STR("reti"), 0xed4d>
, defn<STR("retn"), 0xed45>
>;

using z80_insn_io_l = list<list<>
//
// Input and Output group
//

, defn<STR("in")  , 0xdb  , FMT_X    , INDIR_8>
, defn<STR("in")  , 0xdb  , FMT_X    , REG_A  , INDIR_8>
, defn<STR("in")  , 0xed40, FMT_1W3  , REG    , INDIR_C>
, defn<STR("out") , 0xd3  , FMT_X    , INDIR_8>
, defn<STR("out") , 0xd3  , FMT_X    , INDIR_8, REG_A>
, defn<STR("out") , 0xed41, FMT_X_1W3, INDIR_C, REG>

, defn<STR("ini") , 0xeda2>
, defn<STR("inir"), 0xedb2>
, defn<STR("ind") , 0xedaa>
, defn<STR("indr"), 0xedba>
, defn<STR("outi"), 0xeda3>
, defn<STR("otir"), 0xedb3>
, defn<STR("outd"), 0xedab>
, defn<STR("otdr"), 0xedbb>
>;

using z80_insn_list = list<list<>
                         , z80_insn_ld_l
#if 1
                         , z80_insn_math_l
                         , z80_insn_jmp_l
#endif
                         , z80_insn_io_l
                         >;
}

namespace kas::z80::opc
{
    template <> struct z80_insn_defn_list<OP_Z80_GEN> : gen::z80_insn_list {};
}

#undef STR

#endif
