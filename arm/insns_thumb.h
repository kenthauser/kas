#ifndef KAS_ARM_INSNS_THUMB_H
#define KAS_ARM_INSNS_THUMB_H
//
// Define the ARM instructions. 
//
// For information in the table format, see `target/tgt_mcode_defn_trait.h`
//
// The format names (and naming convention) are in `arm_formats.h`
// The argument validators are in `arm_validate.h`
//
// The common validators with ambiguous names are:
//
// REG          : allow registers

// Convention is for formatter type names to list "shifts" for args in order. 
// shift of `X` indicates arg is not inserted in mach
// for shift if `X`, arg is either determined by "validator" (eg REG_R0)
// or immediate arg validators can normally determine emit size
 
 
#include "arm_insn_common.h"
#include "thumb_formats.h"

namespace kas::arm::opc::thumb
{
#define STR KAS_STRING

using thumb_insn_common_l = list<list<>

// "override" special cases to prefer different encodings
// classic case is "ld <reg>, #0" -> mapped to "clr <reg>"
>;

// Declare THUMB Instructions. Section references from ARM V5 ARchitecture Manual

// ARM5: A6.3
// NB: out-of-range branches handled by linker, not assembler
using thumb_insn_branch_l = list<list<>
, defn<t16_u, STR("b")   , OP<0xe000>, FMT_TJU , BRANCH11>
, defn<t16_c, STR("b")   , OP<0xd000>, FMT_TJC , BRANCH8>
, defn<t16_u, STR("bl")  , OP<0xf000'd000>, FMT_TC, DIRECT>
, defn<t16_u, STR("blx") , OP<0xf000'c000>, FMT_TC, DIRECT>
, defn<t16_u, STR("bx")  , OP<0x4700>, FMT_3I4 , REG>        //4-bit reg #
, defn<t16_u, STR("blx") , OP<0x4780>, FMT_3I4 , REG>
>;

// ARM5: A6.4 Data-processing instructions
// NB: Formats 1-8 process different subset of insns for each
template <typename SZ, typename NAME, unsigned OPC>
using dp_format_1 = list<list<>
// define 3 arg (register) format
, defn<SZ, NAME, OP<0x1800 + (OPC <<  9)>, FMT_0_3_6 , REGL, REGL, REGL>
// allow (and prefer) 3 arg immediate format with first arg doubled
// NB: R_ARM_THM_ABS_Gx_NC relocs used to select byte
// XXX require reloc be specified if not constant
, defn<SZ, NAME, OP<0x3000 + (OPC << 11)>, FMT_8_X_R8, REGL, PREV1, U8>
, defn<SZ, NAME, OP<0x3000 + (OPC << 11)>, FMT_8_R8  , REGL       , U8>
>;

template <typename SZ, typename NAME, unsigned OPC>
using dp_format_2 = 
    defn<SZ, NAME, OP<0x1c00 + (OPC << 9)>, FMT_0_3_6, REGL, REGL, U3>;

// overload "lsl RD, RN, #0" as "movs"
template <typename SZ, typename NAME, unsigned OPC>
using dp_format_2a = 
    defn<SZ, NAME, OP<0x0000 + (OPC << 9)>, FMT_0_3_6, REGL, REGL>;

template <typename SZ, typename NAME, unsigned OPC>
using dp_format_3 = 
    defn<SZ, NAME, OP<0x2000 + (OPC << 11)>, FMT_8_I8, REGL, U8>;

template <typename SZ, typename NAME, unsigned OPC>
using dp_format_4 = 
    defn<SZ, NAME, OP<0x0000 + (OPC << 11)>, FMT_0_3_ABS5, REGL, REGL, U5>;

template <typename SZ, typename NAME, unsigned OPC>
using dp_format_5 = 
    defn<SZ, NAME, OP<0x4000 + (OPC << 6)>, FMT_0_3, REGL, REGL>;

template <typename SZ, typename NAME, unsigned OPC>
using dp_format_6 = list<list<>
, defn<SZ, NAME, OP<0xa000>, FMT_8_PC8 , REGL, DIRECT>
, defn<SZ, NAME, OP<0xa000>, FMT_8_X_I8, REGL, PC, U8_4>
, defn<SZ, NAME, OP<0xa800>, FMT_8_X_I8, REGL, SP, U8_4>
>;

template <typename SZ, typename NAME, unsigned OPC>
using dp_format_7 = 
    defn<SZ, NAME, OP<0xb000 + (OPC << 7)>, FMT_X_I7, SP, U7_4>;

template <typename SZ, typename NAME, unsigned OPC>
using dp_format_8 = 
    defn<SZ, NAME, OP<0x4400 + (OPC << 8)>, FMT_7H0_6H3, REG, REG>;

// ARM V5: A3.4 Data-processing instructions
using thumb_insn_data_l = list<list<>

// overload of LSL RD, RN, #0 -> movs
// list before `LSR` so that it disassembles as MOVS
, dp_format_2a<t16_uS, STR("mov"), 0>

// use `meta-function` to generated related instructions
, dp_format_1<t16_uS, STR("add"),  0>
, dp_format_1<t16_uS, STR("sub"),  1>

, dp_format_2<t16_uS, STR("add"),  0>
, dp_format_2<t16_uS, STR("sub"),  1>

, dp_format_3<t16_uS, STR("mov"),  0>
, dp_format_3<t16_u,  STR("cmp"),  1>
, dp_format_3<t16_uS, STR("add"),  2>
, dp_format_3<t16_uS, STR("sub"),  3>

, dp_format_4<t16_uS, STR("lsl"),  0>
, dp_format_4<t16_uS, STR("lsr"),  1>
, dp_format_4<t16_uS, STR("asr"),  2>

, dp_format_5<t16_uS, STR("and"),  0>
, dp_format_5<t16_uS, STR("eor"),  1>
, dp_format_5<t16_uS, STR("lsl"),  2>
, dp_format_5<t16_uS, STR("lsr"),  3>
, dp_format_5<t16_uS, STR("asr"),  4>
, dp_format_5<t16_uS, STR("adc"),  5>
, dp_format_5<t16_uS, STR("sbc"),  6>
, dp_format_5<t16_uS, STR("ror"),  7>
, dp_format_5<t16_u,  STR("tst"),  8>
, dp_format_5<t16_uS, STR("neg"),  9>
, dp_format_5<t16_u,  STR("cmp"), 10>
, dp_format_5<t16_u,  STR("cmn"), 11>
, dp_format_5<t16_uS, STR("orr"), 12>
, dp_format_5<t16_uS, STR("mul"), 13>
, dp_format_5<t16_uS, STR("bic"), 14>
, dp_format_5<t16_uS, STR("mvn"), 15>

, dp_format_6<t16_u , STR("add"),  0>

, dp_format_7<t16_u , STR("add"),  0>
, dp_format_7<t16_u , STR("sub"),  1>

, dp_format_8<t16_uS, STR("add"),  0>
, dp_format_8<t16_u , STR("cmp"),  1>
, dp_format_8<t16_u , STR("cpy"),  2>    // disassemble as CPY
, dp_format_8<t16_u , STR("mov"),  2>
>;

// ARM5: A6.5 Load and Store Register Instructions
// NB: Formats 1-4 process different subset of insns for each
// Format 1a: allow R_ARM_THB_ABS5 reloc to resolve offset
template <typename SZ, typename NAME, unsigned OPC, typename INDIR_VAL>
using ls_format_1a = 
    defn<SZ, NAME, OP<0x0000 + (OPC << 11)>, FMT_0_ABS5, REGL, INDIR_VAL>;

// Format 1b: ELF reloc not defined
template <typename SZ, typename NAME, unsigned OPC, typename INDIR_VAL>
using ls_format_1b = 
    defn<SZ, NAME, OP<0x0000 + (OPC << 11)>, FMT_0_8X3, REGL, INDIR_VAL>;

template <typename SZ, typename NAME, unsigned OPC>
using ls_format_2 = 
    defn<SZ, NAME, OP<0x0000 + (OPC << 9)>, FMT_0_6X3, REGL, REGL_INDIRL>;

template <typename SZ, typename NAME, unsigned OPC>
using ls_format_3 = list<list<>
  , defn<SZ, NAME, OP<0x4800>, FMT_8_PC8, REGL, DIRECT>
  , defn<SZ, NAME, OP<0x4800>, FMT_8_R8 , REGL, PC_INDIR8>
>;

template <typename SZ, typename NAME, unsigned OPC>
using ls_format_4 = 
    defn<SZ, NAME, OP<0x9000 + (OPC << 11)>, FMT_8_I8, REGL, SP_INDIR8>;


using thumb_insn_load_store_l = list<list<>
// format 1a (relocatable 5-bit word offset)
, ls_format_1a<t16_u , STR("ldr"), 0x0d, REGL_INDIR5_W>
, ls_format_1a<t16_u , STR("str"), 0x0c, REGL_INDIR5_W>

// format 1b (constant 5-bit [byte | halfword] offset)
, ls_format_1b<t16_uB, STR("ldr"), 0x0f, REGL_INDIR5_B>
, ls_format_1b<t16_uH, STR("ldr"), 0x11, REGL_INDIR5_H>
, ls_format_1b<t16_uB, STR("str"), 0x0e, REGL_INDIR5_B>
, ls_format_1b<t16_uH, STR("str"), 0x10, REGL_INDIR5_H>


// format 2
, ls_format_2<t16_u  , STR("ldr"), 0x2c>
, ls_format_2<t16_uB , STR("ldr"), 0x2e>
, ls_format_2<t16_uH , STR("ldr"), 0x2d>
, ls_format_2<t16_uSB, STR("ldr"), 0x2b>
, ls_format_2<t16_uSH, STR("ldr"), 0x2f>

, ls_format_2<t16_u  , STR("str"), 0x28>
, ls_format_2<t16_uB , STR("str"), 0x2a>
, ls_format_2<t16_uH , STR("str"), 0x29>

// format 3/4
, ls_format_3<t16_u, STR("ldr"),  0x0b>
, ls_format_4<t16_u, STR("ldr"),  0x1>
, ls_format_4<t16_u, STR("str"),  0x0>
>;

// ARM5: A6.6 Load and Store Multiple
// ARM V5: A5.4 addressing mode 3: load & store multiple
// ARM suffixes are: IA, IB, DA, DB, { or alises FA, FD, EA, ED }
// THUMB allows: LDMIA and STMIA, and PUSH( = STMDB), and POP( = LDMIA)

using thumb_insn_ls_multiple_l = list<list<>
, defn<t16_uI, STR("ldm") , OP<0xc800>, FMT_8_I8, REGL_WB, REGSET_T> 
, defn<t16_uI, STR("stm") , OP<0xc000>, FMT_8_I8, REGL_WB, REGSET_T> 

, defn<t16_u , STR("push"), OP<0xb400>, FMT_I9, REGSET_T_LR> 
, defn<t16_u , STR("pop") , OP<0xbc00>, FMT_I9, REGSET_T_PC> 
>;

using thumb_insn_exception_l = list<list<>
, defn<t16_u, STR("SWI") , OP<0xdf00>, FMT_I8, U8>
, defn<t16_u, STR("BKPT"), OP<0xbe00>, FMT_I8, U8>
>;


#undef STR

using thumb_gen_v =
             list<list<>
                 , thumb_insn_common_l          // common aliases
                 , thumb_insn_branch_l          // A6.3: branch & related
                 , thumb_insn_data_l            // A6.4: data insns 
                 , thumb_insn_load_store_l      // A6.5: load/store
                 , thumb_insn_ls_multiple_l     // A6.6: load/store misc
                 , thumb_insn_exception_l       // A6.7: exception instructions
                 >;
}

// boilerplate to locate THUMB insns
namespace kas::arm::opc
{
    template <> struct arm_insn_defn_list<OP_ARM_THUMB_16> : thumb::thumb_gen_v {};
}

#endif

