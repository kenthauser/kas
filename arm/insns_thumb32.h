#ifndef KAS_ARM_INSNS_THUMB32_H
#define KAS_ARM_INSNS_THUMB32_H
//
// Define the THUMB-32 instructions. 
//
// Section references are derived from ARM V7-AR Reference
 
#include "arm_insn_common.h"
#include "thumb_formats.h"

namespace kas::arm::opc::thumb32
{
using namespace kas::arm::opc::thumb;
#define STR KAS_STRING

// "override" special cases to prefer different encodings
// classic case is "ld <reg>, #0" -> mapped to "clr <reg>"
using thumb32_insn_common_l = list<list<>
>;

// Declare THUMB32 Instructions. Section references from ARM V7-AR 

// ARM7: A6.3.1 Data-processing instructions (modified immediate)
using thumb32_insn_data_immed_l = list<list<>
, defn<t32_cS, STR("tst"), OP<0xf010'0f00>, FMT_     , REG, THB_IMM12>
, defn<t32_cS, STR("and"), OP<0xf010'0000>, FMT_, REG, NPC, THB_IMM12>
, defn<t32_c , STR("and"), OP<0xf000'0000>, FMT_, REG, REG, THB_IMM12>
, defn<t32_cs, STR("bic"), OP<0xf020'0000>, FMT_, REG, REG, THB_IMM12>
, defn<t32_cs, STR("mov"), OP<0xf04f'0000>, FMT_, REG     , THB_IMM12> 
, defn<t32_cs, STR("orr"), OP<0xf04f'0000>, FMT_, NPC, REG, THB_IMM12> 
, defn<t32_cs, STR("mvn"), OP<0xf06f'0000>, FMT_, REG     , THB_IMM12> 
, defn<t32_cs, STR("orn"), OP<0xf06f'0000>, FMT_, NPC, REG, THB_IMM12> 
, defn<t32_cS, STR("teq"), OP<0xf090'0f00>, FMT_     , REG, THB_IMM12>
, defn<t32_cS, STR("eor"), OP<0xf090'0000>, FMT_, REG, NPC, THB_IMM12>
, defn<t32_c , STR("eor"), OP<0xf080'0000>, FMT_, REG, REG, THB_IMM12>
, defn<t32_cS, STR("cmn"), OP<0xf110'0f00>, FMT_     , REG, THB_IMM12>
, defn<t32_cS, STR("add"), OP<0xf110'0000>, FMT_, REG, NPC, THB_IMM12>
, defn<t32_c , STR("add"), OP<0xf100'0000>, FMT_, REG, REG, THB_IMM12>
, defn<t32_cs, STR("adc"), OP<0xf040'0000>, FMT_, REG, REG, THB_IMM12>
, defn<t32_cs, STR("sbc"), OP<0xf040'0000>, FMT_, REG, REG, THB_IMM12>
, defn<t32_cS, STR("cmp"), OP<0xf1b0'0f00>, FMT_, REG     , THB_IMM12>
, defn<t32_cS, STR("sub"), OP<0xf1b0'0000>, FMT_, REG, NPC, THB_IMM12>
, defn<t32_c , STR("sub"), OP<0xf1a0'0000>, FMT_, REG, REG, THB_IMM12>
, defn<t32_cs, STR("rsb"), OP<0xf1c0'0000>, FMT_, REG, REG, THB_IMM12>
>;

// ARM7: A6.3.3 Data-processing instructions (plain binary immediate)
using thumb32_insn_data_binary_l = list<list<>
, defn<t32_c, STR("adr") , OP<0xf20f'0000>, FMT_, REG, DIRECT>   // direct after
, defn<t32_c, STR("addw"), OP<0xf200'0000>, FMT_, REG, IMM12>
, defn<t32_c, STR("add") , OP<0xf200'0000>, FMT_, REG, IMM12>
, defn<t32_c, STR("movw"), OP<0xf240'0000>, FMT_, REG, IMM16>
, defn<t32_c, STR("mov") , OP<0xf240'0000>, FMT_, REG, IMM16>
, defn<t32_c, STR("adr") , OP<0xf2af'0000>, FMT_, REG, DIRECT>   // direct before
, defn<t32_c, STR("subw"), OP<0xf2a0'0000>, FMT_, REG, IMM12>
, defn<t32_c, STR("sub") , OP<0xf2a0'0000>, FMT_, REG, IMM12>
, defn<t32_c, STR("movt"), OP<0xf2c0'0000>, FMT_, REG, IMM16>
, defn<t32_c, STR("ssat"), OP<0xf300'0000>, FMT_, REG, IMM5, REG, SHIFT>    // XXX
, defn<t32_c, STR("ssat"), OP<0xf300'0000>, FMT_, REG, IMM5, REG>    // XXX
, defn<t32_c, STR("ssat16"), OP<0xf320'0000>, FMT_, REG, IMM1_16, REG>
, defn<t32_c, STR("sbfx"), OP<0xf640'0000>, FMT_, REG, REG, IMM5, IMM1_32>
, defn<t32_c, STR("bfc") , OP<0xf36f'0000>, FMT_, REG     , IMM5, IMM1_32>
, defn<t32_c, STR("bfi") , OP<0xf360'0000>, FMT_, REG, NPC, IMM5, IMM1_32>
, defn<t32_c, STR("usat"), OP<0xf380'0000>, FMT_, REG, IMM5, REG, SHIFT>    // XXX
, defn<t32_c, STR("usat"), OP<0xf380'0000>, FMT_, REG, IMM5, REG>    // XXX
, defn<t32_c, STR("usat16"), OP<0xfb20'0000>, FMT_, REG, IMM1_16, REG>
, defn<t32_c, STR("ubfx"), OP<0xfe40'0000>, FMT_, REG, REG, IMM5, IMM1_32>
>;

// ARM7: A6.3.4 Branches and misc control
using thumb32_insn_branch_l = list<list<>
, defn<t32_n, STR("b"), OP<0xf000'8000>, FMT_ , OFFSET_21>
, defn<t32_c, STR("msr"), OP<0xf380'8020>, FMT_, BANKED_REG, REG>
, defn<t32_c, STR("msr"), OP<0xf380'8000>, FMT_, SPEC_REG, REG> // appl level
, defn<t32_c, STR("msr"), OP<0xf380'8100>, FMT_, SPEC_REG, REG> // sys level
, defn<t32_c, STR("msr"), OP<0xf380'8200>, FMT_, SPEC_REG, REG> // sys level
, defn<t32_u, STR("cps"), OP<0xf3af'8000>, FMT_, IFLAGS>        // CPS<effect>.W
, defn<t32_u, STR("cps"), OP<0xf3af'8000>, FMT_, IFLAGS, IMM5>  // IMM5 = mode
, defn<t32_u, STR("cps"), OP<0xf3af'8000>, FMT_, IMM5>          // IMM5 = mode
, defn<t32_c, STR("nop"), OP<0xf3af'8000>, FMT_>
, defn<t32_c, STR("yield"), OP<0xf3af'8001>, FMT_>
, defn<t32_c, STR("wfe"), OP<0xf3af'8002>, FMT_>
, defn<t32_c, STR("wfi"), OP<0xf3af'8003>, FMT_>
, defn<t32_c, STR("sev"), OP<0xf3af'8004>, FMT_>
, defn<t32_c, STR("csdb"), OP<0xf3af'8014>, FMT_>
, defn<t32_c, STR("dbg"), OP<0xf3af'80f0>, FMT_, IMM4>
, defn<t32_u, STR("leavex"), OP<0xf3bf'8f0f>, FMT_>
, defn<t32_u, STR("enterx"), OP<0xf3bf'8f1f>, FMT_>
, defn<t32_c, STR("clrex"), OP<0xf3bf'8f2f>, FMT_>
, defn<t32_c, STR("dsb"), OP<0xf3bf'8f40>, FMT_, DSB_OPTION>
, defn<t32_c, STR("dbm"), OP<0xf3bf'8f50>, FMT_, DSB_OPTION>
, defn<t32_c, STR("isb"), OP<0xf3bf'8f60>, FMT_, ISB_OPTION>
, defn<t32_c, STR("isb"), OP<0xf3bf'8f6f>, FMT_ >               // default is SY
, defn<t32_c, STR("bxj"), OP<0xf3c0'8f00>, FMT_, REG >
, defn<t32_c, STR("eret"), OP<0xf3de'8f00>, FMT_, REG >
, defn<t32_cS, STR("sub"), OP<0xf3de'8f00>, FMT_, PC, LR, IMMED_8>     // aka ERET
, defn<t32_c, STR("mrs"), OP<0xf3f0'8020>, FMT_, REG, BANKED_REG>
, defn<t32_c, STR("mrs"), OP<0xf3ef'8000>, FMT_, REG, SPEC_REG>     // appl level
, defn<t32_c, STR("mrs"), OP<0xf3ef'8000>, FMT_, REG, SPEC_REG>     // sys level
, defn<t32_u, STR("hvc"), OP<0xf7e0'8000>, FMT_, IMM16>
, defn<t32_c, STR("smc"), OP<0xf7f0'8000>, FMT_, IMM4>
, defn<t32_c, STR("b"), OP<0xf000'8000>, FMT_ , OFFSET_25>
, defn<t32_c, STR("udf"), OP<0xf7f0'a000>, FMT_ , IMM16>
, defn<t32_c, STR("blx"), OP<0xf000'a000>, FMT_ , OFFSET_25>
, defn<t32_c, STR("bl") , OP<0xf000'b000>, FMT_ , OFFSET_25>
>;

// ARM7: A6.3.5 Load Store Multiple
using thumb32_insn_ls_multiple_l = list<list<>
, defn<t32_c , STR("srsdb"), OP<0xe80b'c000>, FMT_, SP   , IMM5>
, defn<t32_c , STR("srsdb"), OP<0xe82b'c000>, FMT_, SP_WB, IMM5>
, defn<t32_c , STR("rfedb"), OP<0xe810'c000>, FMT_, REG> 
, defn<t32_c , STR("rfedb"), OP<0xe830'c000>, FMT_, REG_WB>
// STM is pre-UAL
, defn<t32_ca, STR("stm")  , OP<0xe820'0000>, FMT_, REG   , REGSET>   // M-reg??
, defn<t32_ca, STR("stm")  , OP<0xe8a0'0000>, FMT_, REG_WB, REGSET>   // M-reg??

// NB: need "pop" (single register) to preceed the `...ls_multiple_l` version
// NB: defn moved here from ... `...load_word_l`
, defn<t32_c , STR("pop"), OP<0xf85d'0b04>, FMT_, REGSET_SGL>       // P&M == zero
, defn<t32_c , STR("pop"), OP<0xe8bd'0000>, FMT_        , REGSET>   // P&M reg??
, defn<t32_cd, STR("ldm"), OP<0xe890'0000>, FMT_, REG   , REGSET>   // P&M reg??
, defn<t32_cd, STR("ldm"), OP<0xe8b0'0000>, FMT_, REG_WB, REGSET>   // P&M reg??

// NB: need "push" (single register) to preceed this format
// NB: moved here from ... `...store_single_l`
, defn<t32_c , STR("push"), OP<0xf84d'0d04>, FMT_, REGSET_SGL>      // M == 0
, defn<t32_c , STR("push"), OP<0xe92d'0000>, FMT_, REGSET>          // M-reg??

// STM/LDM are pre-UAL insns
, defn<t32_cd, STR("stm") , OP<0xe920'0000>, FMT_, REG_WB, REGSET>   // M reg??
, defn<t32_cd, STR("stm") , OP<0xe900'0000>, FMT_, REG   , REGSET>   // M reg??
, defn<t32_cD, STR("ldm") , OP<0xe930'0000>, FMT_, REG_WB, REGSET>  // P&M reg??
, defn<t32_cD, STR("ldm") , OP<0xe910'0000>, FMT_, REG   , REGSET>  // P&M reg??

// disassemble: prefer bare opcode it `ia` suffix
, defn<t32_c , STR("srs")  , OP<0xe98b'c000>, FMT_, SP   , IMM5>
, defn<t32_c , STR("srsia"), OP<0xe98b'c000>, FMT_, SP   , IMM5>
, defn<t32_c , STR("srs")  , OP<0xe9ab'c000>, FMT_, SP_WB, IMM5>
, defn<t32_c , STR("srsia"), OP<0xe9ab'c000>, FMT_, SP_WB, IMM5>
, defn<t32_c , STR("rfe")  , OP<0xe990'c000>, FMT_, REG> 
, defn<t32_c , STR("rfeia"), OP<0xe990'c000>, FMT_, REG> 
, defn<t32_c , STR("rfe")  , OP<0xe9b0'c000>, FMT_, REG_WB> 
, defn<t32_c , STR("rfeia"), OP<0xe9b0'c000>, FMT_, REG_WB> 
>;

// ARM7: A6.3.6 Load/store dual/exclusive, table branch
using thumb32_insn_ls_dual_l = list<list<>
// NB: LDRD/STRD is a pre-UAL insn. moved to `...store_single_l`
// NB: LDREX/STREX are UAL-only insns
, defn<t32_c , STR("strex") , OP<0xe840'0000>, FMT_, REG, REG, INDIR_OFFSET8>
, defn<t32_c , STR("ldrex") , OP<0xe850'0f00>, FMT_, REG, INDIR_OFFSET8>
, defn<t32_c , STR("strexb"), OP<0xe8c0'0f40>, FMT_, REG, REG, REG_INDIR>
, defn<t32_c , STR("strexh"), OP<0xe8c0'0f50>, FMT_, REG, REG, REG_INDIR>
, defn<t32_c , STR("strexd"), OP<0xe8c0'0f70>, FMT_, REG, REG, REG_INDIR>
, defn<t32_c , STR("ldrexb"), OP<0xe8d0'0f4f>, FMT_, REG, REG_INDIR>
, defn<t32_c , STR("ldrexh"), OP<0xe8d0'0f5f>, FMT_, REG, REG_INDIR>
, defn<t32_c , STR("ldrexd"), OP<0xe8d0'0f7f>, FMT_, REG, REG_INDIR>

// Memory read is [REG, REG] for TBB, [REG, REG, LSL #1] for TBH
, defn<t32_c , STR("tbb")  , OP<0xe8d0'f000>, FMT_, REG, REG_TBB>
, defn<t32_c , STR("tbh")  , OP<0xe8d0'f010>, FMT_, REG, REG_TBH>
>;

// ARM7: A6.3.7 Load word
using thumb32_insn_load_word_l = list<list<>
// Memory read is [REG, REG, LSR #<imm2>]
, defn<t32_c, STR("ldr"), OP<0xf850'0000>, FMT_, REG, REG_INDIR_W>
// Memory read is [REG, #<imm12>]
, defn<t32_c, STR("ldr"), OP<0xf8d0'0000>, FMT_, REG, INDIR_OFFSET12>
// Memory read is offset, post-indexed, pre-indexed w/ imm8
// NB: P/U/W in LSword
, defn<t32_c , STR("ldr"), OP<0xf850'0800>, FMT_, REG, REG_INDIR>
, defn<t32_cT, STR("ldr"), OP<0xf850'0e00>, FMT_, REG, INDIR_OFFSET8>
// 12-bits of offset with U bit
, defn<t32_c , STR("ldr"), OP<0xf75f'0000>, FMT_, REG, DIRECT>
>;

// ARM7: A6.3.8 Load halfword, memory hints
using thumb32_insn_load_halfword_l = list<list<>
// NB: need to merge halfword/byte/double because of UAL-suffix parsing
// NB: pre-UAL H/SH insns located at end of `...store_single_l`
, defn<t32_c , STR("pld"), OP<0xf81f'f000>, FMT_, DIRECT>
, defn<t32_ct, STR("pld") , OP<0xf890'f000>, FMT_, INDIR_OFFSET12>
, defn<t32_ct, STR("pld") , OP<0xf810'fc00>, FMT_, INDIR_OFFSET8>
, defn<t32_ct, STR("pldw"), OP<0xf8b0'f000>, FMT_, INDIR_OFFSET12>
, defn<t32_ct, STR("pldw"), OP<0xf830'fc00>, FMT_, INDIR_OFFSET8>
, defn<t32_ct, STR("pldw"), OP<0xf810'f000>, FMT_, REG_TBH>

// HT/SHT suffix is UAL only
, defn<t32_c , STR("ldrht") , OP<0xf830'0e00>, FMT_, REG, INDIR_OFFSET8>
, defn<t32_c , STR("ldrsht"), OP<0xf930'0e00>, FMT_, REG, INDIR_OFFSET8>
>;

// ARM7: A6.3.9 Load byte, memory hints
using thumb32_insn_load_byte_l = list<list<>
// NB: need to merge halfword/byte/double because of UAL-suffix parsing
// NB: pre-UAL B/SB insns located at end of `...store_single_l`
, defn<t32_c,  STR("pld") , OP<0xf810'f000>, FMT_, REG_TBH>
, defn<t32_c,  STR("pldw"), OP<0xf830'f000>, FMT_, REG_TBH>
, defn<t32_c,  STR("pld") , OP<0xf81f'f000>, FMT_, INDIR_OFFSET12>
, defn<t32_c,  STR("pldw"), OP<0xf83f'f000>, FMT_, INDIR_OFFSET12>
, defn<t32_c,  STR("pld") , OP<0xf810'fc00>, FMT_, INDIR_OFFSET8>
, defn<t32_c,  STR("pldw"), OP<0xf830'fc00>, FMT_, INDIR_OFFSET8>
, defn<t32_c,  STR("pli") , OP<0xf910'f000>, FMT_, REG_TBH>
, defn<t32_c,  STR("pli") , OP<0xf990'f000>, FMT_, INDIR_OFFSET12>
, defn<t32_c,  STR("pli") , OP<0xf910'fc00>, FMT_, INDIR_OFFSET8>
, defn<t32_c,  STR("pli") , OP<0xf91f'f000>, FMT_, DIRECT>
>;

// ARM7: A6.3.10 Store single item
using thumb32_insn_store_single_l = list<list<>
// NB: need to merge halfword/byte/double because of UAL-suffix parsing
// NB: pre-UAL D insns located at end of `...store_single_l`

// pre-UAL addressable instructions: H/SH/SB/  XXX no D
, defn<t32_cH, STR("ldr"), OP<0xf83f'0000>, FMT_, REG, DIRECT>
, defn<t32_cH, STR("ldr"), OP<0xf8b0'0000>, FMT_, REG, INDIR_OFFSET12>
, defn<t32_cH, STR("ldr"), OP<0xf830'0800>, FMT_, REG, REG_INDIR>
// allow D only
, defn<t32_cD, STR("str") , OP<0xe840'0000>, FMT_, REG, REG, REG_INDIR>
, defn<t32_cD, STR("ldr") , OP<0xe85f'0000>, FMT_, REG, REG, DIRECT>
, defn<t32_cD, STR("ldr") , OP<0xe850'0000>, FMT_, REG, REG, REG_INDIR>

// SH
, defn<t32_cH, STR("ldr"), OP<0xf9b0'0000>, FMT_, REG, INDIR_OFFSET12>
// NB: P/U/W in LSword
, defn<t32_cH, STR("ldr"), OP<0xf930'0800>, FMT_, REG, INDIR_OFFSET8>
// SHT
>;

// ARM7: A6.3.11 Data-processing (shifted register)
using thumb32_insn_data_shifted_l = list<list<>
// immed shift/move in `orr` code-space
, defn<t32_cs, STR("lsl")  , OP<0xea4f'0000>, FMT_, REG, REG, IMM5_NZ>
, defn<t32_cs, STR("mov")  , OP<0xea4f'0000>, FMT_, REG, REG>
, defn<t32_cs, STR("lsr")  , OP<0xea4f'0010>, FMT_, REG, REG, IMM5>
, defn<t32_cs, STR("asr")  , OP<0xea4f'0020>, FMT_, REG, REG, IMM5>
, defn<t32_cs, STR("ror")  , OP<0xea4f'0030>, FMT_, REG, REG, IMM5_NZ>
, defn<t32_cs, STR("rrx")  , OP<0xea4f'0030>, FMT_, REG, REG>

// XXX shift optional for all...duplicate w/o shift after fleshed out
, defn<t32_c , STR("tst")  , OP<0xea10'0f00>, FMT_, REG, REG, SHIFT>
, defn<t32_cs, STR("and")  , OP<0xea00'0000>, FMT_, REG, REG, REG, SHIFT>
, defn<t32_cs, STR("bic")  , OP<0xea20'0000>, FMT_, REG, REG, REG, SHIFT>
, defn<t32_cs, STR("orr")  , OP<0xea40'0000>, FMT_, REG, REG, REG, SHIFT>
, defn<t32_cs, STR("mvn")  , OP<0xea6f'0000>, FMT_, REG, REG, SHIFT>
, defn<t32_cs, STR("orn")  , OP<0xea60'0000>, FMT_, REG, REG, REG, SHIFT>
, defn<t32_c , STR("teq")  , OP<0xea90'0f00>, FMT_, REG, REG, SHIFT>
, defn<t32_cs, STR("eor")  , OP<0xea80'0000>, FMT_, REG, REG, REG, SHIFT>
, defn<t32_c , STR("pkhb") , OP<0xeac0'0020>, FMT_, REG, REG, REG, LSL>
, defn<t32_c , STR("pkhbt"), OP<0xeac0'0000>, FMT_, REG, REG, REG, ASR>
, defn<t32_c , STR("cmn")  , OP<0xeb10'0f00>, FMT_, REG, REG, SHIFT>
, defn<t32_cs, STR("add")  , OP<0xeb00'0000>, FMT_, REG, REG, REG, SHIFT>
, defn<t32_cs, STR("adc")  , OP<0xeb40'0000>, FMT_, REG, REG, REG, SHIFT>
, defn<t32_cs, STR("sbc")  , OP<0xeb60'0000>, FMT_, REG, REG, REG, SHIFT>
, defn<t32_c , STR("cmp")  , OP<0xebb0'0f00>, FMT_, REG, REG, SHIFT>
, defn<t32_cs, STR("sub")  , OP<0xeba0'0000>, FMT_, REG, REG, REG, SHIFT>
, defn<t32_cs, STR("rsb")  , OP<0xebc0'0000>, FMT_, REG, REG, REG, SHIFT>
>;

// ARM7: A6.3.12 Data-processing (register)
using thumb32_insn_data_register_l = list<list<>
, defn<t32_cs, STR("lsl"), OP<0xfa00'f000>, FMT_, REG, REG, REG>
, defn<t32_cs, STR("lsr"), OP<0xfa20'f000>, FMT_, REG, REG, REG>
, defn<t32_cs, STR("asr"), OP<0xfa40'f000>, FMT_, REG, REG, REG>
, defn<t32_cs, STR("ror"), OP<0xfa60'f000>, FMT_, REG, REG, REG>

// XXX Rd and ROR_B are optional for *xt*
, defn<t32_c , STR("sxth")   , OP<0xfa0f'f080>, FMT_, REG, REG, ROR_B>
, defn<t32_c , STR("sxtah")  , OP<0xfa00'f080>, FMT_, REG, REG, REG, ROR_B>
, defn<t32_c , STR("uxth")   , OP<0xfa1f'f080>, FMT_, REG, REG, ROR_B>
, defn<t32_c , STR("uxtah")  , OP<0xfa10'f080>, FMT_, REG, REG, REG, ROR_B>
, defn<t32_c , STR("sxtb16") , OP<0xfa2f'f080>, FMT_, REG, REG, ROR_B>
, defn<t32_c , STR("sxtab16"), OP<0xfa20'f080>, FMT_, REG, REG, REG, ROR_B>
, defn<t32_c , STR("uxtb16") , OP<0xfa3f'f080>, FMT_, REG, REG, ROR_B>
, defn<t32_c , STR("uxtab16"), OP<0xf320'f080>, FMT_, REG, REG, REG, ROR_B>
, defn<t32_c , STR("sxtb")   , OP<0xfa4f'f080>, FMT_, REG, REG, ROR_B>
, defn<t32_c , STR("sxtab")  , OP<0xfa40'f080>, FMT_, REG, REG, REG, ROR_B>
, defn<t32_c , STR("uxtb")   , OP<0xfa5f'f080>, FMT_, REG, REG, ROR_B>
, defn<t32_c , STR("uxtab")  , OP<0xfa50'f080>, FMT_, REG, REG, REG, ROR_B>
>;

// ARM7: A6.3.13 Parallel add/sub, signed
using thumb32_insn_parallel_signed_l = list<list<>
>;


// ARM7: A6.3.14 Parallel add/sub, unsigned
using thumb32_insn_parallel_unsigned_l = list<list<>
>;


// ARM7: A6.3.15 Miscellaneous 
using thumb32_insn_misc_l = list<list<>
>;


// ARM7: A6.3.16 Multiply, Multi-accum, etc
using thumb32_insn_multiply_l = list<list<>
>;

// ARM7: A6.3.17 Long Multiply, Long Multi-accum, etc
using thumb32_insn_long_multiply_l = list<list<>
>;

// ARM7: A6.3.18 Coprocessor, Advanced SIMD, FP
using thumb32_insn_cp_simd_fp_l = list<list<>
>;

#undef STR

// create a list of all insn sub-lists
using thumb32_gen_v =
             list<list<>
                 , thumb32_insn_common_l            // common aliases
                 , thumb32_insn_data_immed_l        // A6.3.1
                 , thumb32_insn_data_binary_l       // A6.3.3
                 , thumb32_insn_branch_l            // A6.3.4
                 , thumb32_insn_ls_multiple_l       // A6.3.5
                 , thumb32_insn_ls_dual_l           // A6.3.6
                 , thumb32_insn_load_word_l         // A6.3.7
                 , thumb32_insn_load_halfword_l     // A6.3.8
                 , thumb32_insn_load_byte_l         // A6.3.9
                 , thumb32_insn_store_single_l      // A6.3.10
                 , thumb32_insn_data_shifted_l      // A6.3.11
                 , thumb32_insn_data_register_l     // A6.3.12
                 , thumb32_insn_parallel_signed_l   // A6.3.13
                 , thumb32_insn_parallel_unsigned_l // A6.3.14
                 , thumb32_insn_misc_l              // A6.3.15
                 , thumb32_insn_multiply_l          // A6.3.16
                 , thumb32_insn_long_multiply_l     // A6.3.17
                 , thumb32_insn_cp_simd_fp_l        // A6.3.18
                 >;
}

// boilerplate to add THUMB_32 insns
namespace kas::arm::opc
{
    template <> struct arm_insn_defn_list<OP_ARM_THUMB_32> : 
                                                thumb32::thumb32_gen_v {};
}

#endif

