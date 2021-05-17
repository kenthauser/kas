#ifndef KAS_M68K_M68000_DEFNS_H
#define KAS_M68K_M68000_DEFNS_H

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
// 5+ validator(s) for m68k arguments, if any. Maximum 6.
//
// The format names (and naming convention) are in `m68k_opcode_formats.h`
// The argument validators are in `m68k_arg_validate.h`


#include "m68k_insn_common.h"

namespace kas::m68k::opc::gen
{
//using namespace common;

#define STR KAS_STRING

///////////////////////////////////////////////////////////////////////////////
//
// `move` instructions (and related)
//

using m68k_move_v = list<list<>
// Define mcode used to process list insns
, defn<sz_list, STR("*LIST*"), OP<0, void, INFO_SIZE_LIST>, FMT_LIST, GEN, GEN>


// general moves
// NB: M68K moves are very regular. But the coldfire arch disallows several moves.
// Examples:
//   1) to memory direct from immed, offset, or index arg
//   2) source & dest are index format,
//   3) when opcode would exceed 3 words.
//   ISA_B allows byte/word immed moves to addr offset, but not index.
//
// Instead of declaring *many* `insns` to handle all cases, the insn is `error`d out
// by a validator in the formatter. This is a difficult insn.
, defn<sz_lwb, STR("move"),  OP<0x0000, void, INFO_SIZE_MOVE>, FMT_0RM_6RM, GEN, ALTERABLE>
, defn<sz_lw , STR("movea"), OP<0x0000, void, INFO_SIZE_MOVE>, FMT_0RM_6RM, GEN, ADDR_REG>

// move quick (three ways to spell it)
, defn<sz_vl, STR("moveq"), OP<0x7000>, FMT_8I_9, Q_IMMED, DATA_REG>
, defn<sz_l , STR("move"),  OP<0x7000>, FMT_8I_9, Q_IMMED, DATA_REG>

// move immediate zero: map to clear
, defn<sz_lwb, STR("move"), OP<0x4200>, FMT_Z_0RM, Z_IMMED, DATA_ALTER>

// map movea 16-bit to LEA
, defn<sz_l,   STR("move") , OP<0x41f8'0000>,  FMT_I16_9, Q_IMMED16, ADDR_REG>
, defn<sz_l,   STR("movea"), OP<0x41f8'0000>,  FMT_I16_9, Q_IMMED16, ADDR_REG>

// CCR, SR moves
, defn<sz_vw, STR("move"), OP<0x46c0>,           FMT_0RM,   DATA, SR>
, defn<sz_vw, STR("move"), OP<0x40c0>,           FMT_X_0RM, SR,   DATA_ALTER>
, defn<sz_vw, STR("move"), OP<0x44c0>,           FMT_0RM,   DATA, CCR>
, defn<sz_vw, STR("move"), OP<0x42c0, read_ccr>, FMT_X_0RM, CCR,  DATA_ALTER>

// move USP
, defn<sz_l, STR("move"), OP<0x4e60>, FMT_0  , ADDR_REG, USP>
, defn<sz_l, STR("move"), OP<0x4e68>, FMT_X_0, USP, ADDR_REG>

// movem (predecrement uses reverse bitset order)
, defn<sz_l, STR("movem"), OP<0x48c0 << 16, m68k>, FMT_I16_0RM, REGSET, CONTROL_ALTER>
, defn<sz_l, STR("movem"), OP<0x48c0 << 16, m68k>, FMT_I16_0RM, REGSET_REV, PRE_DECR>
, defn<sz_l, STR("movem"), OP<0x4cc0 << 16, m68k>, FMT_0RM_I16, CONTROL, REGSET>
, defn<sz_l, STR("movem"), OP<0x4cc0 << 16, m68k>, FMT_0RM_I16, POST_INCR, REGSET>

// movem (coldfire) only mode 2 & 5 (indirect, indirect displacement)
, defn<sz_l, STR("movem"), OP<0x48c0 << 16, coldfire>, FMT_I16_0RM, REGSET, CONTROL_INDIR>
, defn<sz_l, STR("movem"), OP<0x4cc0 << 16, coldfire>, FMT_0RM_I16, CONTROL_INDIR, REGSET>

// movem.w only by m68k
, defn<sz_w, STR("movem"), OP<0x4880 << 16, m68k>, FMT_I16_0RM, REGSET, CONTROL_ALTER>
, defn<sz_w, STR("movem"), OP<0x4880 << 16, m68k>, FMT_I16_0RM, REGSET_REV, PRE_DECR>
, defn<sz_w, STR("movem"), OP<0x4c80 << 16, m68k>, FMT_0RM_I16, CONTROL, REGSET>
, defn<sz_w, STR("movem"), OP<0x4c80 << 16, m68k>, FMT_0RM_I16, POST_INCR, REGSET>

// movep
, defn<sz_lw, STR("movep"), OP<0x0188, movep, INFO_SIZE_WL>, FMT_9_0, DATA_REG, MOVEP>
, defn<sz_lw, STR("movep"), OP<0x0108, movep, INFO_SIZE_WL>, FMT_0_9, MOVEP, DATA_REG>

// privileged moves
, defn<sz_lwb, STR("moves"), OP<0x0e00'0000, m68010>, FMT_28RM_0RM, GEN_REG, MEM_ALTER>
, defn<sz_lwb, STR("moves"), OP<0x0e00'0800, m68010>, FMT_0RM_28RM, MEM_ALTER, GEN_REG>
, defn<sz_vl , STR("movec"), OP<0x4e7a'0000, m68010>, FMT_I12_28RM, CTRL_REG, GEN_REG>
, defn<sz_vl , STR("movec"), OP<0x4e7b'0000, m68010>, FMT_28RM_I12, GEN_REG, CTRL_REG>

// exchange (not on coldfire)
, defn<sz_v  , STR("exg"),  OP<0xc140, m68k>, FMT_9_0, DATA_REG, DATA_REG>
, defn<sz_v  , STR("exg"),  OP<0xc148, m68k>, FMT_9_0, ADDR_REG, ADDR_REG>
, defn<sz_v  , STR("exg"),  OP<0xc188, m68k>, FMT_9_0, ADDR_REG, DATA_REG>
, defn<sz_v  , STR("exg"),  OP<0xc188, m68k>, FMT_0_9, DATA_REG, ADDR_REG>
>;

///////////////////////////////////////////////////////////////////////////////
//
// ALU support "metafunctions" (actually aliases)
// support families of instructions with minimal typing

// math operation families: eg: add, adda, addi
// coldfire doesn't support byte & word forms
// coldfire doesn't support immediate ops to memory destinations
template <uint32_t OPC, uint32_t OPC_IMMED, typename NAME>
using math = list<list<>
// prefer IMMED version if applies
, defn<sz_l  , NAME, OP<OPC_IMMED+0x80, void>, FMT_X_0RM, IMMED, DATA_REG>
, defn<sz_l  , NAME, OP<OPC_IMMED+0x80, m68k>, FMT_X_0RM, IMMED, MEM_ALTER>
, defn<sz_l  , NAME, OP<OPC + 0x080>,    FMT_0RM_9, GEN, DATA_REG>
, defn<sz_l  , NAME, OP<OPC + 0x180>,    FMT_9_0RM, DATA_REG, MEM_ALTER>
, defn<sz_wb , NAME, OP<OPC_IMMED,   m68k>, FMT_X_0RM, IMMED, DATA_ALTER>
, defn<sz_wb , NAME, OP<OPC + 0x000, m68k>, FMT_0RM_9, GEN, DATA_REG>
, defn<sz_wb , NAME, OP<OPC + 0x100, m68k>, FMT_9_0RM, DATA_REG, MEM_ALTER>

, defn<sz_l  , NAME, OP<OPC + 0x1C0, void>, FMT_0RM_9, GEN, ADDR_REG>
, defn<sz_w  , NAME, OP<OPC + 0x0C0, m68k>, FMT_0RM_9, GEN, ADDR_REG>
, defn<sz_l  , string::str_cat<NAME, STR("a")>, OP<OPC + 0x1C0, void>, FMT_0RM_9, GEN, ADDR_REG>
, defn<sz_w  , string::str_cat<NAME, STR("a")>, OP<OPC + 0x0C0, m68k>, FMT_0RM_9, GEN, ADDR_REG>

, defn<sz_l  , string::str_cat<NAME, STR("i")>, OP<OPC_IMMED+0x80, void>, FMT_X_0RM, IMMED, DATA_REG>
, defn<sz_l  , string::str_cat<NAME, STR("i")>, OP<OPC_IMMED+0x80, m68k>, FMT_X_0RM, IMMED, DATA_ALTER>
, defn<sz_wb , string::str_cat<NAME, STR("i")>, OP<OPC_IMMED,      m68k>, FMT_X_0RM, IMMED, DATA_ALTER>
>;

// logical operations: eg: and, andi
// NB: xor is different: no DATA -> DATA_REG mode
template <uint32_t OPC, uint32_t OPC_IMMED, bool is_xor, typename NAME>
using logical = list<list<>
    // prefer IMMED forms
, defn<sz_lwb, NAME, OP<OPC_IMMED>, FMT_X_0RM, IMMED, DATA_ALTER>
, std::conditional_t<is_xor, list<>,
  defn<sz_lwb, NAME, OP<OPC + 0x000>, FMT_0RM_9, DATA, DATA_REG>>
, defn<sz_lwb, NAME, OP<OPC + 0x100>, FMT_9_0RM, DATA_REG, MEM_ALTER>
, defn<sz_lwb, string::str_cat<NAME, STR("i")>, OP<OPC_IMMED>, FMT_X_0RM, IMMED, DATA_ALTER>
>;

// shift instructions: shift left & right are related
template <uint32_t mode, uint32_t dir, typename NAME, typename TST>
using shift_dir = list<list<>
, defn<sz_lwb, NAME, OP<0xe020+(mode<<3)+dir, TST>, FMT_9_0, DATA_REG, DATA_REG>
, defn<sz_lwb, NAME, OP<0xe000+(mode<<3)+dir, TST>, FMT_9_0, Q_MATH, DATA_REG>
, defn<sz_b  , NAME, OP<0xe0c0+(mode<<9)+dir, TST>, FMT_0RM, MEM_ALTER>
>;

template <uint32_t mode, typename NAME, typename TST=void>
using shift = list<list<>
    , shift_dir<mode, 0x100, string::str_cat<NAME, STR("l")>, TST>
    , shift_dir<mode, 0x000, string::str_cat<NAME, STR("r")>, TST>
    >;

// bit operators are regular
template <uint32_t mode, typename TST, typename STATIC, typename DYNAMIC, typename NAME>
using bit = list<list<>
, defn<sz_l, NAME, OP<0x0100+(mode<<6), TST>, FMT_9_0RM, DATA_REG, DATA_REG>
, defn<sz_b, NAME, OP<0x0100+(mode<<6), TST>, FMT_9_0RM, DATA_REG, DYNAMIC>
, defn<sz_l, NAME, OP<0x0100+(mode<<6), TST>, FMT_X_0RM, BIT_IMMED, DATA_REG>
, defn<sz_b, NAME, OP<0x0100+(mode<<6), TST>, FMT_X_0RM, BIT_IMMED, STATIC>
>;

///////////////////////////////////////////////////////////////////////////////

using m68k_math_v = list<list<>
// add/sub/compare instructions
, math<0xd000, 0x0600, STR("add")>
, math<0x9000, 0x0400, STR("sub")>
, math<0xb000, 0x0c00, STR("cmp")>

// additional add/sub/cmp forms
, defn<sz_lwb, STR("clr"),  OP<0x4200>, FMT_0RM, DATA_ALTER>
, defn<sz_lwb, STR("tst"),  OP<0x4a00>, FMT_0RM, GEN>

// long-word (all processors)
, defn<sz_l, STR("addq"), OP<0x5080>, FMT_9_0RM, Q_MATH, ALTERABLE>
, defn<sz_l, STR("add"),  OP<0x5080>, FMT_9_0RM, Q_MATH, ALTERABLE>
, defn<sz_l, STR("subq"), OP<0x5180>, FMT_9_0RM, Q_MATH, ALTERABLE>
, defn<sz_l, STR("sub"),  OP<0x5180>, FMT_9_0RM, Q_MATH, ALTERABLE>
, defn<sz_l, STR("neg"),  OP<0x4480>, FMT_0RM, DATA_ALTER>
, defn<sz_l, STR("not"),  OP<0x4680>, FMT_0RM, DATA_ALTER>

// byte,word (not coldfire)
, defn<sz_wb, STR("addq"), OP<0x5000, m68k>, FMT_9_0RM, Q_MATH, ALTERABLE>
, defn<sz_wb, STR("add"),  OP<0x5000, m68k>, FMT_9_0RM, Q_MATH, ALTERABLE>
, defn<sz_wb, STR("subq"), OP<0x5100, m68k>, FMT_9_0RM, Q_MATH, ALTERABLE>
, defn<sz_wb, STR("sub"),  OP<0x5100, m68k>, FMT_9_0RM, Q_MATH, ALTERABLE>
, defn<sz_wb, STR("neg"),  OP<0x4400, m68k>, FMT_0RM, DATA_ALTER>
, defn<sz_wb, STR("not"),  OP<0x4600, m68k>, FMT_0RM, DATA_ALTER>


// m68k only (not coldfire)
, defn<sz_lwb, STR("cmp"),  OP<0xb108, m68k>, FMT_0_9, POST_INCR, POST_INCR>
, defn<sz_lwb, STR("cmpm"), OP<0xb108, m68k>, FMT_0_9, POST_INCR, POST_INCR>

// multiply & divide (68000 forms)
, defn<sz_vw,  STR("muls"), OP<0xc1c0>, FMT_0RM_9, DATA, DATA_REG>
, defn<sz_vw,  STR("mulu"), OP<0xc0c0>, FMT_0RM_9, DATA, DATA_REG>
, defn<sz_vw,  STR("divs"), OP<0x41c0, divide>, FMT_0RM_9, DATA, DATA_REG>
, defn<sz_vw,  STR("divu"), OP<0x40c0, divide>, FMT_0RM_9, DATA, DATA_REG>

// logical instructions (xor is different: no DATA -> DATA_REG mode)
, logical<0xc000, 0x200, false, STR("and")>
, logical<0x8000, 0x000, false, STR("or")>
, logical<0xb000, 0xa00, true,  STR("eor")>

// shift & bitwise operations
, shift<0, STR("as")>
, shift<1, STR("ls")>
, shift<2, STR("rox"), m68k>
, shift<3, STR("ro"),  m68k>

// bit-wise instructions (m68k insns)
, bit<0, m68k, MEM,       MEM,       STR("btst")>
, bit<1, m68k, MEM_ALTER, MEM_ALTER, STR("bchg")>
, bit<2, m68k, MEM_ALTER, MEM_ALTER, STR("bclr")>
, bit<3, m68k, MEM_ALTER, MEM_ALTER, STR("bset")>

// bit-wise instructions (coldfire insns)
// unique modes allows for bit insns
, bit<0, coldfire, CF_BIT_TST, CF_BIT_STATIC, STR("btst")>
, bit<1, coldfire, MEM_ALTER,  CF_BIT_STATIC, STR("bchg")>
, bit<2, coldfire, MEM_ALTER,  CF_BIT_STATIC, STR("bclr")>
, bit<3, coldfire, MEM_ALTER,  CF_BIT_STATIC, STR("bset")>

// misc alu math instructions (coldfire: long only, dest must be data_reg)
, defn<sz_l, STR("addx"), OP<0xd180, void>, FMT_0_9, DATA_REG, DATA_REG>
, defn<sz_l, STR("addx"), OP<0xd188, m68k>, FMT_0_9, PRE_DECR, PRE_DECR>
, defn<sz_l, STR("subx"), OP<0x9180, void>, FMT_0_9, DATA_REG, DATA_REG>
, defn<sz_l, STR("subx"), OP<0x9188, m68k>, FMT_0_9, PRE_DECR, PRE_DECR>
, defn<sz_l, STR("negx"), OP<0x4080, void>, FMT_0RM, DATA_REG>
, defn<sz_l, STR("negx"), OP<0x4080, m68k>, FMT_0RM, MEM_ALTER>

, defn<sz_wb, STR("addx"), OP<0xd100, m68k>, FMT_0_9, DATA_REG, DATA_REG>
, defn<sz_wb, STR("addx"), OP<0xd108, m68k>, FMT_0_9, PRE_DECR, PRE_DECR>
, defn<sz_wb, STR("subx"), OP<0x9100, m68k>, FMT_0_9, DATA_REG, DATA_REG>
, defn<sz_wb, STR("subx"), OP<0x9108, m68k>, FMT_0_9, PRE_DECR, PRE_DECR>
, defn<sz_wb, STR("negx"), OP<0x4000, m68k>, FMT_0RM, DATA_ALTER>

, defn<sz_lwb, STR("abcd"), OP<0xc100, m68k>, FMT_0_9, DATA_REG, DATA_REG>
, defn<sz_lwb, STR("abcd"), OP<0xc108, m68k>, FMT_0_9, PRE_DECR, PRE_DECR>
, defn<sz_lwb, STR("sbcd"), OP<0x8100, m68k>, FMT_0_9, DATA_REG, DATA_REG>
, defn<sz_lwb, STR("sbcd"), OP<0x8108, m68k>, FMT_0_9, PRE_DECR, PRE_DECR>
, defn<sz_lwb, STR("nbcd"), OP<0x4800, m68k>, FMT_0RM, DATA_ALTER>

// logical ops on SR & CCR
, defn<sz_b,   STR("or"),   OP<0x0000, m68k>, FMT_0RM, IMMED, CCR>
, defn<sz_b,   STR("ori"),  OP<0x0000, m68k>, FMT_0RM, IMMED, CCR>
, defn<sz_w,   STR("or"),   OP<0x0000, void>, FMT_0RM, IMMED, SR>
, defn<sz_w,   STR("ori"),  OP<0x0000, void>, FMT_0RM, IMMED, SR>
, defn<sz_b,   STR("and"),  OP<0x0200, m68k>, FMT_0RM, IMMED, CCR>
, defn<sz_b,   STR("andi"), OP<0x0200, m68k>, FMT_0RM, IMMED, CCR>
, defn<sz_w,   STR("and"),  OP<0x0200, void>, FMT_0RM, IMMED, SR>
, defn<sz_w,   STR("andi"), OP<0x0200, void>, FMT_0RM, IMMED, SR>
, defn<sz_b,   STR("eor"),  OP<0x0a00, m68k>, FMT_0RM, IMMED, CCR>
, defn<sz_b,   STR("eori"), OP<0x0a00, m68k>, FMT_0RM, IMMED, CCR>
, defn<sz_w,   STR("eor"),  OP<0x0a00, void>, FMT_0RM, IMMED, SR>
, defn<sz_w,   STR("eori"), OP<0x0a00, void>, FMT_0RM, IMMED, SR>
>;

///////////////////////////////////////////////////////////////////////////////
//
// Support for instructions which use condition codes
//
// NB: metafunctions also used by CPU_020

template <int value, typename NAME>
struct m68k_cc_trait
{
    using name = NAME;
    using code = std::integral_constant<int, value>;
};

using all_cc_names = list<
          m68k_cc_trait< 0, STR("t")>
        , m68k_cc_trait< 1, STR("f")>
        , m68k_cc_trait< 2, STR("hi")>
        , m68k_cc_trait< 3, STR("ls")>
        , m68k_cc_trait< 4, STR("cc")>        // mit name
        , m68k_cc_trait< 4, STR("hs")>        // motorola name
        , m68k_cc_trait< 5, STR("cs")>        // mit name
        , m68k_cc_trait< 5, STR("lo")>        // motorola name
        , m68k_cc_trait< 6, STR("ne")>
        , m68k_cc_trait< 7, STR("eq")>
        , m68k_cc_trait< 8, STR("vc")>
        , m68k_cc_trait< 9, STR("vs")>
        , m68k_cc_trait<10, STR("pl")>
        , m68k_cc_trait<11, STR("mi")>
        , m68k_cc_trait<12, STR("ge")>
        , m68k_cc_trait<13, STR("lt")>
        , m68k_cc_trait<14, STR("gt")>
        , m68k_cc_trait<15, STR("le")>
        >;

// drop true/false for general cc names
using cc_names = drop_c<all_cc_names, 2>;

// a `callable` to create `insn` for each condition code
template <uint32_t OpCode, typename SZ, typename INFO_SZ, typename NAME, typename TST, typename...Args>
struct apply_cc_trait
{
    template <typename CC>
    using invoke = defn<SZ
                      , string::str_cat<NAME, typename CC::name>
                      , OP<OpCode + (CC::code::value << 8), TST, INFO_SZ>
                      , Args...
                      >;
};

// for CC insns with multiple sizes (ie: trap.[wl]), create list of instructions
template <uint32_t OpCode, typename SZ, typename INFO_SZ, typename CC_LIST, typename...Args>
using cc_sz = transform<CC_LIST, apply_cc_trait<OpCode, SZ, INFO_SZ, Args...>>;

// for single size CC insns (ie `v`) simplified alias
template <uint32_t OpCode, typename CC_LIST, typename...Args>
using cc = cc_sz<OpCode, sz_void, INFO_SIZE_VOID, CC_LIST, Args...>;

///////////////////////////////////////////////////////////////////////////////

using m68k_branch_cc_v = list<list<>

// DBcc -- `dbra` is alternate spelling of `dbf`
, cc<0x50c8, all_cc_names, STR("db"), m68k, FMT_DBCC, DATA_REG, BRANCH>
, defn<sz_v, STR("dbra"), OP<0x51c8, m68k>, FMT_DBCC, DATA_REG, BRANCH>

// jmps & branches
// the 68k user manual defines JMP, JSR, Bcc, BRA, and BSR
// the disassembler picks first match. The assembler picks first shortest
// (ie it will prefer JMP PC@(word) to BRA.w since they're the same size)
, defn<sz_v, STR("jmp"), OP<0x4ec0>, FMT_0RM, CONTROL>
, defn<sz_v, STR("jsr"), OP<0x4e80>, FMT_0RM, CONTROL>

// branch & branch subroutine
, defn<sz_cc, STR("b") ,  OP<0x6000>, FMT_BRANCH, BRANCH_DEL>
, defn<sz_v, STR("bra"),  OP<0x6000>, FMT_BRANCH, BRANCH_DEL>
, defn<sz_v, STR("bsr"),  OP<0x6100>, FMT_BRANCH, BRANCH>

// declare alternate spellings of branch instructions
, defn<sz_cc, STR("j") ,  OP<0x6000>, FMT_BRANCH, BRANCH_DEL>
, defn<sz_v, STR("jra"),  OP<0x6000>, FMT_BRANCH, BRANCH_DEL>
, defn<sz_v, STR("jmp"),  OP<0x6000>, FMT_BRANCH, BRANCH_DEL>

, defn<sz_v, STR("jsr"),  OP<0x6100>, FMT_BRANCH, BRANCH>
, defn<sz_v, STR("jbsr"), OP<0x6100>, FMT_BRANCH, BRANCH>

// Scc (coldfire: data register only)
, cc<0x50c0, all_cc_names, STR("s"), m68k    , FMT_0RM, DATA_ALTER>
, cc<0x50c0, all_cc_names, STR("s"), coldfire, FMT_0RM, DATA_REG>

// trapcc
, defn<sz_v, STR("trap"), OP<0x4e40>, FMT_4I, Q_4BITS>
, defn<sz_v, STR("trapv"), OP<0x4e76, m68k>>

// other branch/returns
, defn<sz_v, STR("rts")    , OP<0x4e75>>
, defn<sz_v, STR("rte")    , OP<0x4e73>>
, defn<sz_v, STR("rtr")    , OP<0x4e77>>
, defn<sz_v, STR("rtd")    , OP<0x4e74, m68010>, void, IMMED>
, defn<sz_v, STR("bkpt")   , OP<0x4848, m68010>, FMT_3I, Q_IMMED>
, defn<sz_v, STR("illegal"), OP<0x4afc>>
>;

using m68k_misc_v = list<list<>
, defn<sz_vl, STR("lea"),  OP<0x41c0>, FMT_0RM_9, CONTROL, ADDR_REG>
, defn<sz_vw, STR("swap"), OP<0x4840>, FMT_0, DATA_REG>
, defn<sz_v , STR("pea"),  OP<0x4840>, FMT_0RM, CONTROL>
, defn<sz_lw, STR("ext"),  OP<0x4880, void, INFO_SIZE_WL>, FMT_0, DATA_REG>
, defn<sz_vb, STR("tas"),  OP<0x4ac0>, FMT_0RM, DATA_ALTER>
, defn<sz_w , STR("chk"),  OP<0x4180, m68k>, FMT_0RM_9, DATA, DATA_REG>

// 68000 user manual lists `link` as "unsized"
, defn<sz_wv, STR("link"), OP<0x4e50>, FMT_0, ADDR_REG, IMMED>
, defn<sz_v , STR("unlk"), OP<0x4e58>, FMT_0, ADDR_REG>
, defn<sz_v , STR("reset"), OP<0x4e70, m68k>>
, defn<sz_v , STR("nop"),  OP<0x4e71>>
, defn<sz_W , STR("stop"), OP<0x4e72'0000>, FMT_I16, IMMED>
>;

#undef STR

using m68k_gen_v = list<list<>
                  , m68k_move_v
                  , m68k_math_v
                  , m68k_branch_cc_v
                  , m68k_misc_v
              >;
}

namespace kas::m68k::opc
{
    template <> struct m68k_insn_defn_list<OP_M68K_GEN> : gen::m68k_gen_v {};
}

#endif
