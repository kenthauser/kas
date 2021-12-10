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
// shift of `X` indicates arg is not inserted in machine code.
// for shift if `X`, arg is either determined by "validator" (eg REG_R0)
// or immediate arg validators can normally determine emit size
 
 
#include "arm_insn_common.h"


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
// NB: ARM_CALL24 validator causes `ARM_CALL` reloc to be emitted
using thumb_insn_branch_l = list<list<>
, defn<t1_c, STR("b")   , OP<0xd000>, FMT_B , THB_JUMP8>
, defn<t1_u, STR("b")   , OP<0xe000>, FMT_B , THB_JUMP11>
, defn<t1_u, STR("bl")  , OP<0xe000>, FMT_B , THB_JUMP11>
, defn<t1_u, STR("blx") , OP<0xe000>, FMT_B , THB_JUMP11>
, defn<t1_c, STR("bx")  , OP<0x4700>, FMT_B , REG>
, defn<t1_c, STR("blx") , OP<0x4780>, FMT_ , REG>
>;

// ARM5: A6.4 Data-processing instructions
// NB: Formats 1-8 process different subset of insns for each
template <typename SZ, typename NAME, unsigned OPC>
using dp_format_1 = 
    defn<SZ, NAME, OP<0x1800 + (OPC << 9)>, FMT_, REGL, REGL, REGL>;

template <typename SZ, typename NAME, unsigned OPC>
using dp_format_2 = 
    defn<SZ, NAME, OP<0x1c00 + (OPC << 9)>, FMT_, REGL, REGL, IMMED_3>;

template <typename SZ, typename NAME, unsigned OPC>
using dp_format_3 = 
    defn<SZ, NAME, OP<0x2000 + (OPC << 11)>, FMT_, REGL, IMMED_8>;

template <typename SZ, typename NAME, unsigned OPC>
using dp_format_4 = 
    defn<SZ, NAME, OP<0x0000 + (OPC << 11)>, FMT_, REGL, REGL, IMMED_5>;

template <typename SZ, typename NAME, unsigned OPC>
using dp_format_5 = 
    defn<SZ, NAME, OP<0x4000 + (OPC << 6)>, FMT_, REGL, REGL>;

template <typename SZ, typename NAME, unsigned OPC>
using dp_format_6 = list<list<>
, defn<SZ, NAME, OP<0xa000>, FMT_, REGL, SP, IMMED_8>
, defn<SZ, NAME, OP<0xa800>, FMT_, REGL, PC, IMMED_8>
>;

template <typename SZ, typename NAME, unsigned OPC>
using dp_format_7 = 
    defn<SZ, NAME, OP<0xb000 + (OPC << 7)>, FMT_, SP, SP, IMMED_7>;

template <typename SZ, typename NAME, unsigned OPC>
using dp_format_8 = 
    defn<SZ, NAME, OP<0x4400 + (OPC << 8)>, FMT_, REG, REG>;

// ARM V5: A3.4 Data-processing instructions
using thumb_insn_data_l = list<list<>
// use `meta-function` to generated related instructions
, dp_format_1<t1_u, STR("add"),  0>
, dp_format_1<t1_u, STR("sub"),  1>

, dp_format_2<t1_u, STR("add"),  0>
, dp_format_2<t1_u, STR("sub"),  1>

, dp_format_3<t1_u, STR("add"),  0>
, dp_format_3<t1_u, STR("sub"),  1>
, dp_format_3<t1_u, STR("mov"),  2>
, dp_format_3<t1_u, STR("cmp"),  3>

, dp_format_4<t1_u, STR("lsl"),  0>
, dp_format_4<t1_u, STR("lsr"),  1>
, dp_format_4<t1_u, STR("asr"),  2>

, dp_format_5<t1_u, STR("mvn"),  0>
, dp_format_5<t1_u, STR("cmp"),  1>
, dp_format_5<t1_u, STR("cmn"),  2>
, dp_format_5<t1_u, STR("tst"),  3>
, dp_format_5<t1_u, STR("adc"),  4>
, dp_format_5<t1_u, STR("sbc"),  5>
, dp_format_5<t1_u, STR("neg"),  6>
, dp_format_5<t1_u, STR("mul"),  7>
, dp_format_5<t1_u, STR("lsl"),  8>
, dp_format_5<t1_u, STR("lsr"),  9>
, dp_format_5<t1_u, STR("asr"), 10>
, dp_format_5<t1_u, STR("ror"), 11>
, dp_format_5<t1_u, STR("and"), 12>
, dp_format_5<t1_u, STR("eor"), 13>
, dp_format_5<t1_u, STR("orr"), 14>
, dp_format_5<t1_u, STR("bic"), 15>

, dp_format_6<t1_u, STR("add"),  0>

, dp_format_7<t1_u, STR("add"),  0>
, dp_format_7<t1_u, STR("sub"),  1>

, dp_format_8<t1_u, STR("mov"),  0>
, dp_format_8<t1_u, STR("add"),  1>
, dp_format_8<t1_u, STR("cmp"),  2>
, dp_format_8<t1_u, STR("cpy"),  3>
>;

// ARM5: A6.5 Load and Store Register Instructions
// NB: Formats 1-4 process different subset of insns for each
template <typename SZ, typename NAME, unsigned OPC>
using ls_format_1 = 
    defn<SZ, NAME, OP<0x0000 + (OPC << 11)>, FMT_, REGL, REGL_INDIR5>;

template <typename SZ, typename NAME, unsigned OPC>
using ls_format_2 = 
    defn<SZ, NAME, OP<0x0000 + (OPC << 9)>, FMT_, REGL, REGL_INDIRL>;

template <typename SZ, typename NAME, unsigned OPC>
using ls_format_3 = 
    defn<SZ, NAME, OP<0x4800>, FMT_, REGL, PC_INDIR8>;

template <typename SZ, typename NAME, unsigned OPC>
using ls_format_4 = 
    defn<SZ, NAME, OP<0x9000 + (OPC << 11)>, FMT_, REGL, SP_INDIR8>;


using thumb_insn_load_store_l = list<list<>
, ls_format_1<t1_u, STR("ldr"),  0>
, ls_format_1<t1_u, STR("str"),  1>

, ls_format_2<t1_u, STR("ldr"),  0>
, ls_format_2<t1_u, STR("str"),  1>

, ls_format_3<t1_u, STR("ldr"),  0>

, ls_format_4<t1_u, STR("ldr"),  0>
, ls_format_4<t1_u, STR("str"),  1>
>;

// ARM5: A6.6 Load and Store Multiple
// NB: Formats 1-2 process different subset of insns for each
template <typename SZ, typename NAME, unsigned OPC>
using lsm_format_1 = 
    defn<SZ, NAME, OP<0xc000 + (OPC << 11)>, FMT_, REGL, REGSET_T>;

template <typename SZ, typename NAME, unsigned OPC>
using lsm_format_2 = 
    defn<SZ, NAME, OP<0xb000 + (OPC << 11)>, FMT_, REGSET_TR>;

// ARM V5: A5.4 addressing mode 3: load & store multiple
// require suffix from: IA, IB, DA, DB, { or alises FA, FD, EA, ED }
using thumb_insn_ls_multiple_l = list<list<>
, lsm_format_1<t1_u, STR("ldm"),  0>
, lsm_format_1<t1_u, STR("stm"),  1>

, lsm_format_2<t1_u, STR("push"),  0>
, lsm_format_2<t1_u, STR("pop") ,  1>
>;

using thumb_insn_exception_l = list<list<>
, defn<t1_u, STR("SWI") , OP<0xdf00>, FMT_, IMMED_8>
, defn<t1_u, STR("BKPT"), OP<0xbe00>, FMT_, IMMED_8>
>;


#undef STR

using thumb_gen_v =
             list<list<>
//                 , thumb_insn_list_l
//                 , thumb_insn_common_l
                 , thumb_insn_branch_l          // A6.3: branch & related
                 , thumb_insn_data_l            // A6.4: data insns 
                 , thumb_insn_load_store_l      // A6.5: load/store
                 , thumb_insn_ls_multiple_l     // A6.6: load/store misc
                 , thumb_insn_exception_l       // A6.7
                 >;
}

// boilerplate to locate THUMB insns
namespace kas::arm::opc
{
    template <> struct arm_insn_defn_list<OP_ARM_THUMB> : thumb::thumb_gen_v {};
}

#endif

