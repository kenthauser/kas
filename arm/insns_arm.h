#ifndef KAS_ARM_INSNS_ARM_H
#define KAS_ARM_INSNS_ARM_H
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


namespace kas::arm::opc::gen
{
#define STR KAS_STRING

// Dummy machine-code for "list" opcode
using arm_insn_list_l = list<list<>
, defn<void, STR("*LIST*"), OP<0, void, arm_info_list>, FMT_LIST>//, REG>
>;

using arm_insn_common_l = list<list<>

// "override" special cases to prefer different encodings
// classic case is "ld <reg>, #0" -> mapped to "clr <reg>"
>;

// Declare ARM 5 Instructions. Section references from ARM V5 ARchitecture Manual

// ARM5: A3.3
// NB: out-of-range branches handled by linker, not assembler
// NB: ARM_CALL24 validator causes `ARM_CALL` reloc to be emitted
using arm_insn_branch_l = list<list<>
, defn<a7_c, STR("b")   , OP<0x0a00'0000>, FMT_B , ARM_JUMP24>
, defn<a7_u, STR("bl")  , OP<0xeb00'0000>, FMT_B , ARM_CALL24>
, defn<a7_c, STR("bl")  , OP<0x0b00'0000>, FMT_B , ARM_JUMP24>
, defn<a7_u, STR("blx") , OP<0xfa00'0000>, FMT_B , ARM_CALL24>
, defn<a7_c, STR("blx") , OP<0x012f'ff30>, FMT_0 , REG>
, defn<a7_c, STR("bx")  , OP<0x012f'ff10>, FMT_BX, REG>     // emit ARM_V4BX reloc
, defn<a7_c, STR("bxj") , OP<0x012f'ff20>, FMT_0,  REG>
>;

// ARM V5: A5.1 addressing mode 1 - Data-processing operands
// `data_processing` is for two registers + shifter_operand
template <typename SZ, typename NAME, unsigned ARM_OP>
using data_processing = list<list<>
// match 3 patterns (IMM8_4, REG, REG+SHIFT)
, defn<SZ, NAME, OP<0x200'0000 | (ARM_OP << 21)>
                , FMT_12_16_F, REG, REG, IMM8_4>
, defn<SZ, NAME, OP<0x000'0000 | (ARM_OP << 21)>
                , FMT_12_16_0, REG, REG, REG> 
, defn<SZ, NAME, OP<0x000'0000 | (ARM_OP << 21)>
                , FMT_12_16_0_S, REG, REG, REG, SHIFT> 
>;

template <typename SZ, typename NAME, unsigned ARM_OP>
using dp_add_sub = list<list<>
// match 3 patterns (IMM8_4, REG, REG+SHIFT)
// NB: let KBFD handle negative arguments via RELOC
, defn<SZ, NAME, OP<0x200'0000 | (ARM_OP << 21)>
                , FMT_12_16_AS, REG, REG, IMM8_4>
, defn<SZ, NAME, OP<0x000'0000 | (ARM_OP << 21)>
                , FMT_12_16_0, REG, REG, REG> 
, defn<SZ, NAME, OP<0x000'0000 | (ARM_OP << 21)>
                , FMT_12_16_0_S, REG, REG, REG, SHIFT> 
>;

// `dp_one` is `data_processing` formats with single reg plus shifter_operand
// NB: mov (et.al) use DST = 12, reg at 16 = SBZ
// NB: tst (et.al) use DST = 16, reg at 12 = SBZ
template <typename SZ, typename NAME, unsigned ARM_OP, typename FMT, typename FMT_F, typename FMT_S>
using dp_one = list<list<>
// match 3 patterns (IMM8_4, REG, REG+SHIFT)
, defn<SZ, NAME, OP<0x200'0000 | (ARM_OP << 21)>
                , FMT_F, REG, IMM8_4>
, defn<SZ, NAME, OP<0x000'0000 | (ARM_OP << 21)>
                , FMT, REG, REG> 
, defn<SZ, NAME, OP<0x000'0000 | (ARM_OP << 21)>
                , FMT_S, REG, SHIFT> 
>;

// ARM V5: A3.4 Data-processing instructions
using arm_insn_data_l = list<list<>
// use `meta-function` to generated related instructions
, data_processing<a7_cs, STR("and"),  0>
, data_processing<a7_cs, STR("eor"),  1>
, data_processing<a7_cs, STR("rsb"),  3>
, data_processing<a7_cs, STR("adc"),  5>
, data_processing<a7_cs, STR("sbc"),  6>
, data_processing<a7_cs, STR("rsc"),  7>
, data_processing<a7_cs, STR("orr"), 12>
, data_processing<a7_cs, STR("bic"), 14>

// use add/subtract reloc as required
, dp_add_sub<a7_cs, STR("sub"),  2>
, dp_add_sub<a7_cs, STR("add"),  4>

// No Rd, S-implied & not allowed
, dp_one<a7_c , STR("tst"),  8, FMT_16_0, FMT_16_F, FMT_16_0_S>
, dp_one<a7_c , STR("teq"),  9, FMT_16_0, FMT_16_F, FMT_16_0_S>
, dp_one<a7_c , STR("cmp"), 10, FMT_16_0, FMT_16_F, FMT_16_0_S>
, dp_one<a7_c , STR("cmn"), 11, FMT_16_0, FMT_16_F, FMT_16_0_S>

// No Rn, S allowed
, dp_one<a7_cs, STR("mov"), 13, FMT_12_0, FMT_12_F, FMT_12_0_S>
, dp_one<a7_cs, STR("mvn"), 15, FMT_12_0, FMT_12_F, FMT_12_0_S>
>;

// ARM V5: A5.2 addressing mode 2: Load & Store word or unsigned byte
// load word & unsigned byte
using arm_insn_load_store_l = list<list<>
// load/store: allow `B` suffix for unsigned byte
, defn<a7_cb , STR("ldr") , OP<0x410'0000>, FMT_LD, REG, REG_INDIR>
, defn<a7_cb , STR("str") , OP<0x400'0000>, FMT_LD, REG, REG_INDIR>

// load user: require suffix T or BT (post-index formats only)
, defn<a7_cT , STR("ldr") , OP<0x410'0000>, FMT_LD, REG, POST_INDEX>
, defn<a7_cT , STR("str") , OP<0x400'0000>, FMT_LD, REG, POST_INDEX>

// preload data (offset formats only)
, defn<a7_u  , STR("pld") , OP<0x550'0f00>, FMT_LD, INDIR_OFFSET>
>;

// ARM V5: A5.3 addressing mode 3: Miscellaneous loads & stores
// require suffix from: H, SH, SB, D
using arm_insn_ls_misc_l = list<list<>
, defn<a7_cHs, STR("ldr"), OP<0x000'0090>, FMT_LDH, REG, LS_MISC>
, defn<a7_cHs, STR("str"), OP<0x010'0090>, FMT_LDH, REG, LS_MISC>
>;

// ARM V5: A5.4 addressing mode 3: load & store multiple
// require suffix from: IA, IB, DA, DB, { or alises FA, FD, EA, ED }
using arm_insn_ls_multiple_l = list<list<>
, defn<a7_c , STR("push"), OP<0x52d'0004>, FMT_12, REGSET_SGL>
, defn<a7_c , STR("push"), OP<0x92d'0000>, FMT_RS, REGSET>
, defn<a7_c , STR("pop") , OP<0x49d'0004>, FMT_12, REGSET_SGL>
, defn<a7_c , STR("pop") , OP<0x8bd'0000>, FMT_RS, REGSET>
, defn<a7_cM, STR("ldm"), OP<0x810'0000>, FMT_LDM, REG       , REGSET>
, defn<a7_cM, STR("ldm"), OP<0x850'0000>, FMT_LDM, REG       , REGSET_USER>
, defn<a7_cM, STR("ldm"), OP<0x830'0000>, FMT_LDM, REG_UPDATE, REGSET>
, defn<a7_cM, STR("ldm"), OP<0x870'0000>, FMT_LDM, REG_UPDATE, REGSET_USER>
, defn<a7_cM, STR("stm"), OP<0x800'0000>, FMT_LDM, REG       , REGSET>
, defn<a7_cM, STR("stm"), OP<0x840'0000>, FMT_LDM, REG       , REGSET_USER>
, defn<a7_cM, STR("stm"), OP<0x820'0000>, FMT_LDM, REG_UPDATE, REGSET>
, defn<a7_cM, STR("stm"), OP<0x860'0000>, FMT_LDM, REG_UPDATE, REGSET_USER>
, defn<a7_uM, STR("srs"), OP<0xf84d'0500, ARMv6>, FMT_I00B5, IMM5>
, defn<a7_uM, STR("srs"), OP<0xf86d'0500, ARMv6>, FMT_I00B5, IMM5_UPDATE>
, defn<a7_uM, STR("rfe"), OP<0xf810'0a00, ARMv6>, FMT_16, REG>
, defn<a7_uM, STR("rfe"), OP<0xf830'0a00, ARMv6>, FMT_16, REG_UPDATE>
>;

// ARM V5: A5.5 addressing mode 4: load & store coprocessor
using arm_insn_ls_coprocessor = list<list<>
, defn<a7_cl, STR("ldc")  , OP<0x0c10'0000>, FMT_8_12_M4, COPROC, CREG, CP_REG_INDIR>
, defn<a7_ul, STR("ldc2") , OP<0xfc10'0000>, FMT_8_12_M4, COPROC, CREG, CP_REG_INDIR>

, defn<a7_cl, STR("stc")  , OP<0x0c00'0000>, FMT_8_12_M4, COPROC, CREG, CP_REG_INDIR>
, defn<a7_ul, STR("stc2") , OP<0xfc00'0000>, FMT_8_12_M4, COPROC, CREG, CP_REG_INDIR>

, defn<a7_c , STR("cdp")  , OP<0x0e00'0000>, FMT_8_20_12_16_0_05B3
                                , COPROC, U4, CREG, CREG, CREG, U4>
, defn<a7_u , STR("cdp2") , OP<0xfe00'0000>, FMT_8_20_12_16_0_05B3
                                , COPROC, U4, CREG, CREG, CREG, U4>
, defn<a7_c , STR("mcr")  , OP<0x0e00'0010>, FMT_8_20_12_16_0
                                , COPROC, U4, REG, CREG, CREG>
, defn<a7_c , STR("mcr")  , OP<0x0e00'0010>, FMT_8_20_12_16_0_05B3
                                , COPROC, U4, REG, CREG, CREG, U4>
, defn<a7_u , STR("mcr2") , OP<0xfe00'0010>, FMT_8_20_12_16_0
                                , COPROC, U4, REG, CREG, CREG>
, defn<a7_u , STR("mcr2") , OP<0xfe00'0010>, FMT_8_20_12_16_0_05B3
                                , COPROC, U4, REG, CREG, CREG, U4>
, defn<a7_c , STR("mcrr") , OP<0x0c40'0000>, FMT_8_20_12_16_0
                                , COPROC, U4, REG, REG, CREG>
, defn<a7_u , STR("mcrr2"), OP<0xfc40'0000>, FMT_8_20_12_16_0
                                , COPROC, U4, REG, REG, CREG>
, defn<a7_c , STR("mrc")  , OP<0x0e10'0010>, FMT_8_20_12_16_0
                                , COPROC, U4, REG, CREG, CREG>
, defn<a7_c , STR("mrc")  , OP<0x0e10'0010>, FMT_8_20_12_16_0_05B3
                                , COPROC, U4, REG, CREG, CREG, U4>
, defn<a7_u , STR("mrc2") , OP<0xfe10'0010>, FMT_8_20_12_16_0
                                , COPROC, U4, REG, CREG, CREG>
, defn<a7_u , STR("mrc2") , OP<0xfe10'0010>, FMT_8_20_12_16_0_05B3
                                , COPROC, U4, REG, CREG, CREG, U4>
, defn<a7_c , STR("mrrc") , OP<0x0c50'0000>, FMT_8_20_12_16_0
                                , COPROC, U4, REG, REG, CREG>
, defn<a7_u , STR("mrrc2"), OP<0xfc50'0000>, FMT_8_20_12_16_0
                                , COPROC, U4, REG, REG, CREG>
>;

// ARM V5: A3.16.7 Unconditional instructions
using arm_insn_unconditional_l = list<list<>
, defn<a7_u, STR("cpsie") , OP<0xf108'0000, ARMv6>, FMT_4_0, FLAGS_AIF, IMM4>
, defn<a7_u, STR("cpsid") , OP<0xf10c'0000, ARMv6>, FMT_4_0, FLAGS_AIF, IMM4>
, defn<a7_u, STR("cpsie") , OP<0xf108'0000, ARMv6>, FMT_4_0, FLAGS_AIF>
, defn<a7_u, STR("cpsid") , OP<0xf10c'0000, ARMv6>, FMT_4_0, FLAGS_AIF>
, defn<a7_u, STR("cps")   , OP<0xf102'0000, ARMv6>, FMT_0  , IMM4>
, defn<a7_u, STR("setend"), OP<0xf101'0000, ARMv6>, FMT_I09B1, FLAGS_ENDIAN>
// PLD defined as addressing mode 2 insn
// SRS and RFE defined with addressing mode 4 insn
// BLX defined with branch modes
// MCRR2, MRRC2, STC2, LDC2, CDP2, MCR2, MRC2 defined with coprocessor INSNs
>;

// bottom/top meta arg
using BT = list<pair<STR("b"), int_<0>>, pair<STR("t"),int_<1>>>;

// A3.5 Multiply & Multiply Accumulate
using arm_insn_multiply_l = list<list<>
// 32x32 -> 32
, defn<a7_cs, STR("mul")    , OP<0x000'0090> , FMT_16_0_8_12, REG, REG, REG>
, defn<a7_cs, STR("mla")    , OP<0x020'0090> , FMT_16_0_8_12, REG, REG, REG, REG>

// 32x32 -> 64
, defn<a7_cs, STR("smull")  , OP<0x700'0090> , FMT_12_16_0_8, REG, REG, REG, REG>
, defn<a7_cs, STR("umull")  , OP<0x080'0090> , FMT_12_16_0_8, REG, REG, REG, REG>
, defn<a7_cs, STR("smlal")  , OP<0x700'0050> , FMT_12_16_0_8, REG, REG, REG, REG>
, defn<a7_cs, STR("umlal")  , OP<0x0a0'0090> , FMT_12_16_0_8, REG, REG, REG, REG>
, defn<a7_c,  STR("umaal")  , OP<0x040'0090, ARMv6>
                                             , FMT_12_16_0_8, REG, REG, REG, REG>

// 16x16 -> 32
// smul<x><y>
, defn<a7_c,  STR("smulbb") , OP<0x400'0080> , FMT_16_0_8_12, REG, REG, REG, REG>
, defn<a7_c,  STR("smulbt") , OP<0x400'00c0> , FMT_16_0_8_12, REG, REG, REG, REG>
, defn<a7_c,  STR("smultb") , OP<0x400'00a0> , FMT_16_0_8_12, REG, REG, REG, REG>
, defn<a7_c,  STR("smultt") , OP<0x400'00e0> , FMT_16_0_8_12, REG, REG, REG, REG>

// smla<x><y>
, defn<a7_c,  STR("smlabb") , OP<0x400'0080> , FMT_16_0_8_12, REG, REG, REG, REG>
, defn<a7_c,  STR("smlabt") , OP<0x400'00c0> , FMT_16_0_8_12, REG, REG, REG, REG>
, defn<a7_c,  STR("smlatb") , OP<0x400'00a0> , FMT_16_0_8_12, REG, REG, REG, REG>
, defn<a7_c,  STR("smlatt") , OP<0x400'00e0> , FMT_16_0_8_12, REG, REG, REG, REG>

// smlal<x><y>
, defn<a7_c,  STR("smlalbb"), OP<0x140'0080> , FMT_12_16_0_8, REG, REG, REG, REG>
, defn<a7_c,  STR("smlalbt"), OP<0x140'00c0> , FMT_12_16_0_8, REG, REG, REG, REG>
, defn<a7_c,  STR("smlaltb"), OP<0x140'00a0> , FMT_12_16_0_8, REG, REG, REG, REG>
, defn<a7_c,  STR("smlaltt"), OP<0x140'00e0> , FMT_12_16_0_8, REG, REG, REG, REG>

// 32x16 -> 32
, defn<a7_c,  STR("smulwb") , OP<0x120'00a0> , FMT_16_0_8, REG, REG, REG>
, defn<a7_c,  STR("smulwt") , OP<0x120'00e0> , FMT_16_0_8, REG, REG, REG>

, defn<a7_c,  STR("smlawb") , OP<0x120'0080> , FMT_16_0_8_12, REG, REG, REG, REG>
, defn<a7_c,  STR("smlawt") , OP<0x120'00c0> , FMT_16_0_8_12, REG, REG, REG, REG>

// 32x32 -> 32 most significant word
, defn<a7_c,  STR("smmul")  , OP<0x750'0f10> , FMT_16_0_8, REG, REG, REG>
, defn<a7_c,  STR("smmulr") , OP<0x750'0f30> , FMT_16_0_8, REG, REG, REG>

, defn<a7_c,  STR("smmla")  , OP<0x750'0010> , FMT_16_0_8_12, REG, REG, REG, REG>
, defn<a7_c,  STR("smmlar") , OP<0x750'0030> , FMT_16_0_8_12, REG, REG, REG, REG>

, defn<a7_c,  STR("smmls")  , OP<0x750'00d0> , FMT_16_0_8_12, REG, REG, REG, REG>
, defn<a7_c,  STR("smmlsr") , OP<0x750'00f0> , FMT_16_0_8_12, REG, REG, REG, REG>

// dual halfword multiply
, defn<a7_c,  STR("smuad")  , OP<0x700'0f10, ARMv6> , FMT_16_0_8, REG, REG, REG>
, defn<a7_c,  STR("smuadx") , OP<0x700'0f30, ARMv6> , FMT_16_0_8, REG, REG, REG>

, defn<a7_c,  STR("smusd")  , OP<0x700'f050, ARMv6> , FMT_16_0_8, REG, REG, REG>
, defn<a7_c,  STR("smusdx") , OP<0x700'f070, ARMv6> , FMT_16_0_8, REG, REG, REG>

, defn<a7_c,  STR("smlad")  , OP<0x700'0010, ARMv6> , FMT_16_0_8_12, REG, REG, REG, REG>
, defn<a7_c,  STR("smladx") , OP<0x700'0030, ARMv6> , FMT_16_0_8_12, REG, REG, REG, REG>

, defn<a7_c,  STR("smlsd")  , OP<0x700'0050, ARMv6> , FMT_12_16_0_8, REG, REG, REG, REG>
, defn<a7_c,  STR("smlsdx") , OP<0x700'0070, ARMv6> , FMT_12_16_0_8, REG, REG, REG, REG>

, defn<a7_c,  STR("smlald") , OP<0x740'0010, ARMv6> , FMT_16_0_8_12, REG, REG, REG, REG>
, defn<a7_c,  STR("smlaldx"), OP<0x740'0030, ARMv6> , FMT_16_0_8_12, REG, REG, REG, REG>

, defn<a7_c,  STR("smlsld") , OP<0x740'0050, ARMv6> , FMT_12_16_0_8, REG, REG, REG, REG>
, defn<a7_c,  STR("smlsldx"), OP<0x740'0070, ARMv6> , FMT_12_16_0_8, REG, REG, REG, REG>
>;


template <typename LIST, unsigned shift, typename...Ts>
using defn_add_pfx = defn<Ts...>;


// prefixes for the parallel add/sub instructions
// prefixes: S, Q, SH, U UQ, UH
//SADD16  = 0x610'0f10
//QADD16  = 0x620'0f10
//SHADD16 = 0x630'0f10
//UADD16  = 0x650'0f10
//UQADD16 = 0x660'0f10
//UHADD16 = 0x670'0f10

using PADD_SUB_PFXS = list<
      pair<int_<1>, STR("s")>
    , pair<int_<2>, STR("q")>
    , pair<int_<3>, STR("sh")>
    , pair<int_<5>, STR("u")>
    , pair<int_<6>, STR("uq")>
    , pair<int_<7>, STR("uh")>
    >;

template <typename...Ts>
using padd_defn = defn_add_pfx<PADD_SUB_PFXS, 20, Ts...>;

// A3.6 Parallel addition and subtraction instructions
using arm_insn_parallel_add_l = list<list<>

, padd_defn<a7_c, STR("add16")  , OP<0x600'0f10>, FMT_12_16_0, REG, REG, REG>
, padd_defn<a7_c, STR("addsubx"), OP<0x600'0f30>, FMT_12_16_0, REG, REG, REG>
, padd_defn<a7_c, STR("subaddx"), OP<0x600'0f50>, FMT_12_16_0, REG, REG, REG>
, padd_defn<a7_c, STR("sub16")  , OP<0x600'0f70>, FMT_12_16_0, REG, REG, REG>
, padd_defn<a7_c, STR("add8")   , OP<0x600'0f90>, FMT_12_16_0, REG, REG, REG>
, padd_defn<a7_c, STR("sub8")   , OP<0x600'0ff0>, FMT_12_16_0, REG, REG, REG>
>;

// prefixes for the extend instructions
// sxtab16 = 0x680'0070
// uxtab16 = 0x6c0'0070

using XTND_PFXS = list<
      pair<int_<0>, STR("s")>
    , pair<int_<1>, STR("u")>
    >;

template <typename...Ts>
using xtend_defn = defn_add_pfx<XTND_PFXS, 22, Ts...>;

// A3.7 Extend instructions
using arm_insn_extend_l = list<list<>
// prefixes: S, U
, xtend_defn<a7_c, STR("xtab16"), OP<0x680'0070, ARMv6>, FMT_12_16_0_8B2, REG, REG, REG> 
, xtend_defn<a7_c, STR("xtab16"), OP<0x680'0070, ARMv6>, FMT_12_16_0_8B2, REG, REG, REG, XTEND_ROR> 
, xtend_defn<a7_c, STR("xtab"), OP<0x6a0'0070, ARMv6>, FMT_12_16_0_8B2, REG, REG, REG> 
, xtend_defn<a7_c, STR("xtab"), OP<0x6a0'0070, ARMv6>, FMT_12_16_0_8B2, REG, REG, REG, XTEND_ROR> 
, xtend_defn<a7_c, STR("xtah"), OP<0x6b0'0070, ARMv6>, FMT_12_16_0_8B2, REG, REG, REG> 
, xtend_defn<a7_c, STR("xtah"), OP<0x6b0'0070, ARMv6>, FMT_12_16_0_8B2, REG, REG, REG, XTEND_ROR> 
, xtend_defn<a7_c, STR("xtb"), OP<0x6af'0070, ARMv6>, FMT_12_0_8B2, REG, REG> 
, xtend_defn<a7_c, STR("xtb"), OP<0x6af'0070, ARMv6>, FMT_12_0_8B2, REG, REG, XTEND_ROR> 
, xtend_defn<a7_c, STR("xth"), OP<0x6bf'0070, ARMv6>, FMT_12_0_8B2, REG, REG> 
, xtend_defn<a7_c, STR("xth"), OP<0x6bf'0070, ARMv6>, FMT_12_0_8B2, REG, REG, XTEND_ROR> 
>;

// A3.8 Miscellaneous arithmetic
// A3.9 Other miscellaneous instructions
using arm_insn_misc_l = list<list<>
, defn<a7_cs, STR("clz")   , OP<0x16f'0f10, void >, FMT_12_0, REG, REG>
, defn<a7_cs, STR("usad8") , OP<0x780'f010, ARMv6>, FMT_16_0_8, REG, REG, REG>
, defn<a7_cs, STR("usada8"), OP<0x780'0010, ARMv6>, FMT_16_0_8_12, REG, REG, REG, REG>
, defn<a7_cs, STR("pkhbt") , OP<0x680'0010, ARMv6>, FMT_12_16_0_S, REG, REG, REG>
, defn<a7_cs, STR("pkhbt") , OP<0x680'0010, ARMv6>, FMT_12_16_0_S, REG, REG, REG, LSL>
, defn<a7_cs, STR("pkhtb") , OP<0x680'0050, ARMv6>, FMT_12_16_0_S, REG, REG, REG>
, defn<a7_cs, STR("pkhtb") , OP<0x680'0050, ARMv6>, FMT_12_16_0_S, REG, REG, REG, ASR>
, defn<a7_cs, STR("rev")   , OP<0x6bf'0f30, ARMv6>, FMT_12_0, REG, REG>
, defn<a7_cs, STR("rev16") , OP<0x6bf'0fb0, ARMv6>, FMT_12_0, REG, REG>
, defn<a7_cs, STR("revsh") , OP<0x6ff'0fb0, ARMv6>, FMT_12_0, REG, REG>
, defn<a7_cs, STR("sel")   , OP<0x680'0fb0, ARMv6>, FMT_12_16_0, REG, REG, REG>

, defn<a7_cs, STR("ssat")  , OP<0x6a0'0010, ARMv6>, FMT_12_B_0_S, REG, BIT, REG>
, defn<a7_cs, STR("ssat")  , OP<0x6a0'0010, ARMv6>, FMT_12_B_0_S, REG, BIT, LSL>
, defn<a7_cs, STR("ssat")  , OP<0x6a0'0050, ARMv6>, FMT_12_B_0_S, REG, BIT, ASR>
, defn<a7_cs, STR("usat")  , OP<0x6e0'0010, ARMv6>, FMT_12_B_0_S, REG, BIT, REG>
, defn<a7_cs, STR("usat")  , OP<0x6e0'0010, ARMv6>, FMT_12_B_0_S, REG, BIT, LSL>
, defn<a7_cs, STR("usat")  , OP<0x6e0'0050, ARMv6>, FMT_12_B_0_S, REG, BIT, ASR>

, defn<a7_cs, STR("ssat16"), OP<0x6a0'0f30, ARMv6>, FMT_12_16_0, REG, BIT4, REG>
, defn<a7_cs, STR("usat16"), OP<0x6e0'0f30, ARMv6>, FMT_12_16_0, REG, BIT4, REG>
>;

// A3.10 Status register access instructions
using arm_insn_status_l = list<list<>
, defn<a7_c, STR("mrs"), OP<0x10f'0000>, FMT_12, REG, CPSR>
, defn<a7_c, STR("mrs"), OP<0x14f'0000>, FMT_12, REG, SPSR>
, defn<a7_c, STR("msr"), OP<0x320'f000>, FMT_16_F, CSPR_FLAGS, IMM8_4>
, defn<a7_c, STR("msr"), OP<0x360'f000>, FMT_16_F, SSPR_FLAGS, IMM8_4>
, defn<a7_c, STR("msr"), OP<0x120'f000>, FMT_16_0, CSPR_FLAGS, REG>
, defn<a7_c, STR("msr"), OP<0x160'f000>, FMT_16_0, SSPR_FLAGS, REG>
// cps:    defined as unconditional
// setend: defined as unconditional
>;

// A3.13 Semaphore instructions
// A3.14 Exception-generating instructions
using arm_insn_semaphore_l = list<list<>
, defn<a7_cb, STR("swp") , OP<0x100'0090>, FMT_12_0_16, REG, REG>
, defn<a7_cb, STR("swp") , OP<0x100'0090>, FMT_12_0_16, REG, REG, REG>

, defn<a7_c, STR("swi") , OP<0x0f00'0000>, FMT_I24, IMM24>
, defn<a7_u, STR("bkpt"), OP<0xe120'0070>, FMT_BKPT, IMM16>
>;


#undef STR

using arm_gen_v =
             list<list<>
                 , arm_insn_list_l
//                 , arm_insn_common_l        // prefered mappings: eg ld <reg>, #0 -> clr <reg>
                 , arm_insn_data_l          // A5.1: data insns 
                 , arm_insn_load_store_l    // A5.2: load/store
                 , arm_insn_ls_misc_l       // A5.3: load/store misc
                 , arm_insn_ls_multiple_l   // A5.4: load/store misc
                 
                 , arm_insn_branch_l        // A3.3: branch & related
                 , arm_insn_multiply_l      // A3.5 Multiply & Multiply Accumulate
#if 0
                 , arm_insn_media_l         // A5.4: media insns
                 , arm_insn_cp_supv_l       // A5.6: co-processor and supervisor
                 , arm_insn_uncond_l        // A5.7: unconditional insns
#endif
                 >;
}

// boilerplate to locate ARM5 insns
namespace kas::arm::opc
{
    template <> struct arm_insn_defn_list<OP_ARM_GEN> : gen::arm_gen_v {};
}

#endif

