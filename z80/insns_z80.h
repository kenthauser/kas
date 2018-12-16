#ifndef KAS_Z80_M68000_DEFNS_H
#define KAS_Z80_M68000_DEFNS_H

// While the tables look cryptic, the MPL fields are generally as follows:
//
// 1. opcode name. The metafunction modifies the base name
//     eg. lwb<STR("move") expands to "move.l", "move.w", "move.b"
//         (and/or "movel", "movew", moveb") Base opcode size field value also
//         updated with l/w/b size value
//
// 2. base opcode. 16-bit or 32-bit opcode value
//
// 3. functor which returns enum to select which (virtual) subclass
//          of `core::opcode` should be used for this instruction.
//          returns zero if opcode not supported according to
//          run time switches (eg 68020 instruction in 68000 mode)
//
// 4. format index value on where values are inserted in the base opcode
//          (register numbers, address modes, immediate values, etc)
//
// 5+ validator(s) for z80 arguments, if any. Maximum 6.
//
// The format names (and naming convention) are in `z80_opcode_formats.h`
// The argument validators are in `z80_arg_validate.h`


#include "z80_insn_common.h"

namespace kas::z80::opc::gen
{
//using namespace common;

#define STR KAS_STRING


using z80_insn_ld_l = list<list<>
//
// 8-bit load group
//

, defn<STR("ld"), 0x40, FMT_3_0, REG    , REG_GEN>
, defn<STR("ld"), 0x40, FMT_3_0, REG_GEN, REG>
, defn<STR("ld"), 0x06, FMT_3  , REG_GEN, IMMED_8>
, defn<STR("ld"), 0x46, FMT_3  , REG_GEN, INDIR>
, defn<STR("ld"), 0x46, FMT_X_3, INDIR  , REG_GEN>
, defn<STR("ld"), 0x06, FMT_X  , INDIR  , IMMED_8>

, defn<STR("ld"), 0x0a, FMT_X_4, REG_A, INDIR_DBL>
, defn<STR("ld"), 0x02, FMT_4  , INDIR_DBL, REG_A>

, defn<STR("ld"), 0x3a, FMT_X  , REG_A, DIRECT>
, defn<STR("ld"), 0x32, FMT_X  , DIRECT, REG_A>

, defn<STR("ld"), 0xed57, FMT_X, REG_A, REG_I>
, defn<STR("ld"), 0xed5f, FMT_X, REG_A, REG_R>
, defn<STR("ld"), 0xed47, FMT_X, REG_I, REG_A>
, defn<STR("ld"), 0xed4f, FMT_X, REG_R, REG_A>

//
// 16-bit load group
//

, defn<STR("ld"), 0x01, FMT_4, REG_DBL, IMMED_16>
, defn<STR("ld"), 0x21, FMT_X, REG_IDX, IMMED_16>

// XXX NB: IDX_HL also allows IX, IY
, defn<STR("ld"), 0x2a  , FMT_X, IDX_HL , DIRECT>
, defn<STR("ld"), 0xed4b, FMT_4, REG_DBL, DIRECT>

, defn<STR("ld"), 0x22  , FMT_X  , DIRECT, IDX_HL>
, defn<STR("ld"), 0xed43, FMT_X_4, DIRECT, REG_DBL>

, defn<STR("ld"), 0xf9, FMT_X, REG_SP, IDX_HL>

, defn<STR("push"), 0xc5, FMT_4, REG_DBL_AF>
, defn<STR("pop") , 0xc1, FMT_4, REG_DBL_AF>

, defn<STR("push"), 0xe5, FMT_X, REG_IDX>
, defn<STR("pop") , 0xe1, FMT_X, REG_IDX>

// exchange, block transfer, search group

, defn<STR("ex")  , 0xeb, FMT_X, REG_DE, REG_HL>
, defn<STR("ex")  , 0x08, FMT_X, REG_AF, REG_AF>
, defn<STR("exaf"), 0x08>
, defn<STR("exx") , 0xd9>

, defn<STR("ex")  , 0xe3, FMT_X, INDIR_SP, IDX_HL>

, defn<STR("ldi") , 0xeda0>
, defn<STR("ldir"), 0xedb0>
, defn<STR("ldd") , 0xeda8>
, defn<STR("lddr"), 0xedb8>
, defn<STR("cpi") , 0xeda1>
, defn<STR("cpir"), 0xedb1>
, defn<STR("cpd") , 0xeda9>
, defn<STR("cpdr"), 0xedb9>
>;


// accumulator formats
// 1) add a,b
// 2) add b     (a implied)
// 3) add a,(hl) (idx+n allowed)
// 4) add (hl)  (a implied)
// 5) add a,#4
// 6) add #4    (a implied)

// declare metafunction to declare class of operations
template <typename NAME, uint32_t OPC, uint32_t OPC_IMMED>
using math = list<list<>
    , defn<NAME, OPC      , FMT_X_0  , REG_A, REG_GEN>
    , defn<NAME, OPC      , FMT_0    , REG_GEN>
    , defn<NAME, OPC      , FMT_IDX_0, REG_A, IDX_HL>
    , defn<NAME, OPC      , FMT_IDX_0, IDX_HL>
    , defn<NAME, OPC_IMMED, FMT_X    , REG_A, IMMED_8>
    , defn<NAME, OPC_IMMED, FMT_X    , IMMED_8>
    >;

using z80_insn_math_l = list<list<>
//
// 8-bit arithmetic group
//
, math<STR("add"), 0x80, 0xc6>
, math<STR("adc"), 0x88, 0xce>
, math<STR("sub"), 0x90, 0xd6>
, math<STR("sbc"), 0x98, 0xde>
, math<STR("and"), 0xa0, 0xe6>
, math<STR("xor"), 0xa8, 0xee>
, math<STR("or") , 0xb0, 0xf6>
, math<STR("cp") , 0xb8, 0xfe>

, defn<STR("inc"), 0x04, FMT_3, REG_GEN>
, defn<STR("inc"), 0x34, FMT_X, IDX_HL>        // pick up IDX
, defn<STR("dec"), 0x05, FMT_3, REG_GEN>
, defn<STR("dec"), 0x35, FMT_X, IDX_HL>


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
, defn<STR("im")  , 0xed46, FMT_IM, IMMED_012>

//
// 16-bit arithmetic group
//

// NB: only self add of dbl accumuators allowed. ie no add ix, hl
, defn<STR("add"), 0x09, FMT_X, IDX_HL, REG_BC>
, defn<STR("add"), 0x19, FMT_X, IDX_HL, REG_DE>
, defn<STR("add"), 0x39, FMT_X, IDX_HL, REG_SP>
, defn<STR("add"), 0x29, FMT_X, REG_HL, REG_HL>
, defn<STR("add"), 0x29, FMT_X, REG_IX, REG_IX>
, defn<STR("add"), 0x29, FMT_X, REG_IY, REG_IY>

, defn<STR("inc"), 0x03, FMT_4, REG_DBL>
, defn<STR("inc"), 0x23, FMT_X, REG_IDX>        // pick up IDX
, defn<STR("dec"), 0x0b, FMT_4, REG_DBL>
, defn<STR("dec"), 0x2b, FMT_X, REG_IDX>

, defn<STR("adc"), 0xed4a, FMT_X_4, REG_HL, REG_DBL>
, defn<STR("sbc"), 0xed42, FMT_X_4, REG_HL, REG_DBL>
// 
// Rotate & shift group
//

// 8080 instructions
, defn<STR("rlca"), 0x07>
, defn<STR("rrca"), 0x0f>
, defn<STR("rla") , 0x17>
, defn<STR("rra") , 0x1f>

// Z80 shifts
, defn<STR("rlc") , 0xcb00, FMT_0, REG_GEN>
, defn<STR("rrc") , 0xcb08, FMT_0, REG_GEN>
, defn<STR("rl")  , 0xcb10, FMT_0, REG_GEN>
, defn<STR("rr")  , 0xcb18, FMT_0, REG_GEN>
, defn<STR("sla") , 0xcb20, FMT_0, REG_GEN>
, defn<STR("sra") , 0xcb28, FMT_0, REG_GEN>
, defn<STR("srl") , 0xcb38, FMT_0, REG_GEN>

, defn<STR("rld"), 0xed6f>
, defn<STR("rrd"), 0xed67>

//
// Bit set, reset, and test group
//

, defn<STR("bit"), 0xcb40, FMT_3_0, BIT_NUM, REG_GEN>
, defn<STR("res"), 0xcb80, FMT_3_0, BIT_NUM, REG_GEN>
, defn<STR("set"), 0xcbc0, FMT_3_0, BIT_NUM, REG_GEN>
>;

using z80_insn_jmp_l = list<list<>
//
// Jump group
//

, defn<STR("jp")  , 0xc3, FMT_X , DIRECT>
, defn<STR("jp")  , 0xc2, FMT_3 , CC, DIRECT>
, defn<STR("jp")  , 0xe9, FMT_X , IDX_HL>
, defn<STR("jr")  , 0x18, FMT_JR, DIRECT>
, defn<STR("jr")  , 0x20, FMT_JR, JR_CC, DIRECT>
, defn<STR("djnz"), 0x10, FMT_JR, DIRECT>

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

, defn<STR("in")  , 0xdb  , FMT_X  , INDIR>
, defn<STR("in")  , 0xdb  , FMT_X  , REG_A  , INDIR>
, defn<STR("in")  , 0xed40, FMT_3  , REG    , INDIR_C>
, defn<STR("out") , 0xd3  , FMT_X  , INDIR>
, defn<STR("out") , 0xd3  , FMT_X  , INDIR  , REG_A>
, defn<STR("out") , 0xed41, FMT_X_3, INDIR_C, REG>

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
