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

using arm_insn_list_l = list<defn<void, STR("*LIST*"), OP<0>, FMT_LIST, REG>>;

using arm_insn_common_l = list<list<>

// Dummy machine-code for "list" opcode
//, defn<void, STR("*LIST*"), OP<0>, FMT_LIST, REG, REG>

// "override" special cases to prefer different encodings
// classic case is "ld reg, #0" -> mapped to "clr <reg>"

// XXX SUB/ADD with PC: form PC-relative address

// assemble `stm sp!, ...` as push
, defn<a7_cs, STR("stm") , OP<0x92d'0000>, FMT_X, SP_UPDATE, REGSET>
// assemble `ldm sp!, ...` as pop
, defn<a7_cs, STR("ldm") , OP<0x8ad'0000>, FMT_X, SP_UPDATE, REGSET>

// XXX temp
, defn<a7_cs, STR("b")  , OP<0xa00'0000>, FMT_X, LABEL>
, defn<void, STR("push") , OP<0x8001>, FMT_X   , REG>
, defn<void, STR("pop")  , OP<0x8001>, FMT_X   , REG>
, defn<void, STR("ldr")  , OP<0x8001>, FMT_X   , REG, REG>

//, defn<a7_cs, STR("mov") , OP<0x1a0'0000, void, void, 0xf000 >, FMT_12_0   , REG, REG> 
//, defn<a7_u  , STR("mov") , OP<0x1a0'0000, void, void, 0xf000 >, FMT_12_0   , REG, REG> 
//, defn<a7_cs, STR("mov") , OP<0x3a0'0000, void, void, 0xf000 >, FMT_12_F   , REG, U12> 
//, defn<a7_c , STR("movw"), OP<0x300'0000, hw::v6t2           >, FMT_12_MOVW, REG, U16>
//, defn<a7_c , STR("mov") , OP<0x300'0000, hw::v6t2           >, FMT_12_MOVW, REG, U16>
// disassembles as LSR, ASR, etc.
//, defn<a7_c , STR("mov") , OP<0xF00'0000                     >, FMT_12_0_F, REG, REG, SHIFT>

//, defn<a7_cb, STR("ldr"), OP<0x410'0000>, FMT_12_F, REG, INDIR>
//, defn<a7_cb, STR("ldr"), OP<0x41f'0000>, FMT_12_F, REG, LABEL>
//, defn<a7_cb, STR("str"), OP<0x400'0000>, FMT_12_F, REG, INDIR>
//, defn<a7_cb, STR("str"), OP<0x40f'0000>, FMT_12_F, REG, LABEL>


>;
// Declare ARM Instructions. Section references from ARM V5 ARchitecture Manual

// ARM5: A3.3
using arm_insn_branch_l = list<list<>
, defn<a7_c, STR("b")   , OP<0xa00'0000>, FMT_X, LABEL>
, defn<a7_c, STR("bl")  , OP<0xb00'0000>, FMT_X, LABEL>
, defn<a7_u, STR("blx") , OP<0xa00'0000>, FMT_X, LABEL>     // XXX B24 = H-Bit?
, defn<a7_c, STR("blx") , OP<0x120'0030>, FMT_0, REG>
, defn<a7_c, STR("bx")  , OP<0x120'0010>, FMT_0, REG>
, defn<a7_u, STR("bxj") , OP<0x120'0020>, FMT_0, REG>
>;

// NB: base code for register data-processing INSNs is zero.
//     base code for immediate versions is 0x200'0000
//  `data_processing` is for general format: 2 registers plus shifter_operand
// addressing mode 1: Data-processing Instructions
template <typename SZ, typename NAME, unsigned ARM_OP>
using data_processing = list<list<>
// match 4 patterns
, defn<SZ, NAME, OP<0x000'0000 | (ARM_OP << 21)>
                , FMT_X, REG, REG, REG, SHIFT> 
, defn<SZ, NAME, OP<0x000'0000 | (ARM_OP << 21)>
                , FMT_X, REG, REG, REG> 
, defn<SZ, NAME, OP<0x000'0000 | (ARM_OP << 21)>
                , FMT_X, REG, REG, SHIFT> 
, defn<SZ, NAME, OP<0x200'0000 | (ARM_OP << 21)>
                , FMT_X, REG, REG, IMM12>
>;

// `dp_one` is `data_processing` formats with single reg plus shifter_operand
template <typename SZ, typename NAME, unsigned ARM_OP, typename FMT, typename FMT_S>
using dp_one = list<list<>
// match 4 patterns
, defn<SZ, NAME, OP<0x000'0000 | (ARM_OP << 21)>
                , FMT_S, REG, REG, SHIFT> 
, defn<SZ, NAME, OP<0x000'0000 | (ARM_OP << 21)>
                , FMT, REG, REG> 
, defn<SZ, NAME, OP<0x000'0000 | (ARM_OP << 21)>
                , FMT, REG, SHIFT> 
, defn<SZ, NAME, OP<0x200'0000 | (ARM_OP << 21)>
                , FMT, REG, IMM12>
>;

// Data Processing: ARM-7 A+R section 5.2, pg A5-195
using arm_insn_data_l = list<list<>
// use `meta-function` to generated related instructions
// ARM V5: A3.4
, data_processing<a7_cs, STR("and"),  0>
, data_processing<a7_cs, STR("eor"),  1>
, data_processing<a7_cs, STR("sub"),  2>
, data_processing<a7_cs, STR("rsb"),  3>
, data_processing<a7_cs, STR("add"),  4>
, data_processing<a7_cs, STR("adc"),  5>
, data_processing<a7_cs, STR("sbc"),  6>
, data_processing<a7_cs, STR("rsc"),  7>
, data_processing<a7_cs, STR("orr"), 12>
, data_processing<a7_cs, STR("bic"), 14>

// No Rd, S-implied & not allowed
, dp_one<a7_c , STR("tst"),  8, FMT_16_0, FMT_16_0_S>
, dp_one<a7_c , STR("teq"),  9, FMT_16_0, FMT_16_0_S>
, dp_one<a7_c , STR("cmp"), 10, FMT_16_0, FMT_16_0_S>
, dp_one<a7_c , STR("cmn"), 11, FMT_16_0, FMT_16_0_S>

// No Rn, S allowed
, dp_one<a7_cs, STR("mov"), 13, FMT_12_0, FMT_12_0_S>
, dp_one<a7_cs, STR("mvn"), 15, FMT_12_0, FMT_12_0_S>
>;

// Load & stores
using arm_insn_load_store_l = list<list<>
// addressing mode 2: Load & Store word or unsigned byte
// load word & unsigned byte
, defn<a7_cb , STR("ldr") , OP<0x410'0000>, FMT_LD, REG, REG_INDIR>
, defn<a7_cb , STR("str") , OP<0x400'0000>, FMT_LD, REG, REG_INDIR>

// load user; require suffix T or BT
, defn<a7_cT , STR("ldr") , OP<0x410'0000>, FMT_LD, REG, POST_INDEX>
, defn<a7_cT , STR("str") , OP<0x400'0000>, FMT_LD, REG, POST_INDEX>

// preload data
, defn<a7_u  , STR("pld") , OP<0x550'0f00>, FMT_PLD, OFFSET12>

// addressing mode 3: Miscellaneous loads & stores
// require suffix from: H, SH, SB, D
, defn<a7_cHs, STR("ldr"), OP<0x000'0090>, FMT_LDH, REG, OFFSET8>
, defn<a7_cHs, STR("str"), OP<0x000'0090>, FMT_LDH, REG, OFFSET8>

// addressing mode 4: Load & Store Multiple
// required suffix from: IA, IB, DA, DB, { or alises FA, FD, EA, ED }
, defn<a7_cM , STR("ldm"), OP<0x810'0000>, FMT_LDM, REG       , REGSET>
, defn<a7_cM , STR("ldm"), OP<0x830'0000>, FMT_LDM, REG_UPDATE, REGSET>
, defn<a7_cM , STR("stm"), OP<0x800'0000>, FMT_LDM, REG       , REGSET>
, defn<a7_cM , STR("stm"), OP<0x820'0000>, FMT_LDM, REG_UPDATE, REGSET>
>;

#if 0

#if 0
// XXX No IMMED for LSL, etc...
, dp_one<sz_s, STR("lsl"), 13, 0, SHIFT_NZ>
, dp_one<sz_s, STR("lsr"), 26, 1>
, dp_one<sz_s, STR("asr"), 26, 2>
, dp_one<sz_s, STR("rrx"), 26, 3, SHIFT_Z>
, dp_one<sz_s, STR("ror"), 26, 3, SHIFT_NZ>
#endif

// A3.5 Multiply & Multiply Accumulate
, defn<a7_cs, STR("mla")    , OP<0x020'0090> , FMT_16_0_8_12, REG, REG, REG, REG>
, defn<a7_cs, STR("mul")    , OP<0x000'0090> , FMT_16_0_8, REG, REG, REG>
// smla<x><y>
, defn<a7_c,  STR("smlabb") , OP<0x400'0080> , FMT_16_0_8_12, REG, REG, REG, REG>
, defn<a7_c,  STR("smlabt") , OP<0x400'00c0> , FMT_16_0_8_12, REG, REG, REG, REG>
, defn<a7_c,  STR("smlatb") , OP<0x400'00a0> , FMT_16_0_8_12, REG, REG, REG, REG>
, defn<a7_c,  STR("smlatt") , OP<0x400'00e0> , FMT_16_0_8_12, REG, REG, REG, REG>

, defn<a7_c,  STR("smlad")  , OP<0x700'0010> , FMT_16_0_8_12, REG, REG, REG, REG>
, defn<a7_c,  STR("smladx") , OP<0x700'0030> , FMT_16_0_8_12, REG, REG, REG, REG>

, defn<a7_cs, STR("smlal")  , OP<0x700'0050> , FMT_12_16_0_8, REG, REG, REG, REG>
// smlal<x><y>
, defn<a7_c,  STR("smlalbb"), OP<0x140'0080> , FMT_12_16_0_8, REG, REG, REG, REG>
, defn<a7_c,  STR("smlalbt"), OP<0x140'00c0> , FMT_12_16_0_8, REG, REG, REG, REG>
, defn<a7_c,  STR("smlaltb"), OP<0x140'00a0> , FMT_12_16_0_8, REG, REG, REG, REG>
, defn<a7_c,  STR("smlaltt"), OP<0x140'00e0> , FMT_12_16_0_8, REG, REG, REG, REG>

, defn<a7_c,  STR("smlald") , OP<0x740'0010> , FMT_16_0_8_12, REG, REG, REG, REG>
, defn<a7_c,  STR("smlaldx"), OP<0x740'0030> , FMT_16_0_8_12, REG, REG, REG, REG>

, defn<a7_c,  STR("smlawb") , OP<0x120'0080> , FMT_16_0_8_12, REG, REG, REG, REG>
, defn<a7_c,  STR("smlawt") , OP<0x120'00c0> , FMT_16_0_8_12, REG, REG, REG, REG>

, defn<a7_c,  STR("smlsd")  , OP<0x700'0050> , FMT_12_16_0_8, REG, REG, REG, REG>
, defn<a7_c,  STR("smlsdx") , OP<0x700'0070> , FMT_12_16_0_8, REG, REG, REG, REG>

, defn<a7_c,  STR("smlsld") , OP<0x740'0050> , FMT_12_16_0_8, REG, REG, REG, REG>
, defn<a7_c,  STR("smlsldx"), OP<0x740'0070> , FMT_12_16_0_8, REG, REG, REG, REG>

, defn<a7_c,  STR("smmla")  , OP<0x750'0010> , FMT_16_0_8_12, REG, REG, REG, REG>
, defn<a7_c,  STR("smmlar") , OP<0x750'0030> , FMT_16_0_8_12, REG, REG, REG, REG>

, defn<a7_c,  STR("smmls")  , OP<0x750'00d0> , FMT_16_0_8_12, REG, REG, REG, REG>
, defn<a7_c,  STR("smmlsr") , OP<0x750'00f0> , FMT_16_0_8_12, REG, REG, REG, REG>

, defn<a7_c,  STR("smmul")  , OP<0x750'0f10> , FMT_16_0_8, REG, REG, REG>
, defn<a7_c,  STR("smmulr") , OP<0x750'0f30> , FMT_16_0_8, REG, REG, REG>

, defn<a7_c,  STR("smuad")  , OP<0x700'0f10> , FMT_16_0_8, REG, REG, REG>
, defn<a7_c,  STR("smuadx") , OP<0x700'0f30> , FMT_16_0_8, REG, REG, REG>
// smlal<x><y>
, defn<a7_c,  STR("smulbb") , OP<0x160'0080> , FMT_12_16_0_8, REG, REG, REG, REG>
, defn<a7_c,  STR("smulbt") , OP<0x160'00c0> , FMT_12_16_0_8, REG, REG, REG, REG>
, defn<a7_c,  STR("smultb") , OP<0x160'00a0> , FMT_12_16_0_8, REG, REG, REG, REG>
, defn<a7_c,  STR("smultt") , OP<0x160'00e0> , FMT_12_16_0_8, REG, REG, REG, REG>

, defn<a7_cs, STR("smull")  , OP<0x700'0090> , FMT_12_16_0_8, REG, REG, REG, REG>

, defn<a7_c,  STR("smulwb") , OP<0x120'00a0> , FMT_16_0_8, REG, REG, REG>
, defn<a7_c,  STR("smulwt") , OP<0x120'00e0> , FMT_16_0_8, REG, REG, REG>

, defn<a7_c,  STR("smusd")  , OP<0x700'f050> , FMT_16_0_8, REG, REG, REG>
, defn<a7_c,  STR("smusdx") , OP<0x700'f070> , FMT_16_0_8, REG, REG, REG>

, defn<a7_c,  STR("umaal")  , OP<0x040'0090> , FMT_12_16_0_8, REG, REG, REG, REG>
, defn<a7_cs, STR("umlal")  , OP<0x0a0'0090> , FMT_12_16_0_8, REG, REG, REG, REG>
, defn<a7_cs, STR("umull")  , OP<0x080'0090> , FMT_12_16_0_8, REG, REG, REG, REG>

// A5.2.8 Extra Load/store instructions
// XXX
// A5.2.9 Extra Load/store instructions, unprivileged
// XXX
// A5.2.10 Sychronization primitives
,defn<a7_cs, STR("swp")   , OP<0x100'0090>      , FMT_X, REG, REG, REG_INDIR>
,defn<a7_cs, STR("swpb")  , OP<0x140'0090>      , FMT_X, REG, REG, REG_INDIR>
,defn<a7_cs, STR("strex") , OP<0x180'0090, void>, FMT_X, REG, REG, REG_OFFSET>
,defn<a7_cs, STR("ldrex") , OP<0x190'0f9f, void>, FMT_X, REG, REG_OFFSET>
,defn<a7_cs, STR("strexd"), OP<0x1a0'0f90, void>, FMT_X, REG, REG, REG, REG_INDIR>
,defn<a7_cs, STR("ldrexd"), OP<0x1b0'0f9f, void>, FMT_X, REG, REG, REG_INDIR>
,defn<a7_cs, STR("strexb"), OP<0x1c0'0f90, void>, FMT_X, REG, REG, REG_INDIR>
,defn<a7_cs, STR("ldrexb"), OP<0x1d0'0f9f, void>, FMT_X, REG, REG_INDIR>
,defn<a7_cs, STR("strexh"), OP<0x1c0'0f90, void>, FMT_X, REG, REG, REG_INDIR>
,defn<a7_cs, STR("ldrexh"), OP<0x1d0'0f9f, void>, FMT_X, REG, REG_INDIR>

// A5.2.11 MSR (immediate), and hints
,defn<a7_cs, STR("nop")   , OP<0x320'f000, void>, FMT_X>
,defn<a7_cs, STR("yield") , OP<0x320'f001, void>, FMT_X>
,defn<a7_cs, STR("wfe")   , OP<0x320'f002, void>, FMT_X>
,defn<a7_cs, STR("wfi")   , OP<0x320'f003, void>, FMT_X>
,defn<a7_cs, STR("sev")   , OP<0x320'f004, void>, FMT_X>
,defn<a7_cs, STR("csdb")  , OP<0x320'f014, void>, FMT_X>
,defn<a7_cs, STR("dbg")   , OP<0x320'f0f0, void>, FMT_X, IMM4>

// XXX need work on inserters
,defn<a7_cs, STR("msr")   , OP<0x324'f000>      , FMT_X, APSR, IMM12>
,defn<a7_cs, STR("msr")   , OP<0x328'f000>      , FMT_X, CPSR, IMM12>

,defn<a7_cs, STR("msr")   , OP<0x320'f000>      , FMT_X, APSR, IMM12>
,defn<a7_cs, STR("msr")   , OP<0x320'f000>      , FMT_X, CPSR, IMM12>
,defn<a7_cs, STR("msr")   , OP<0x320'f000>      , FMT_X, SPSR, IMM12>

,defn<a7_cs, STR("msr")   , OP<0x360'f000>      , FMT_X, APSR, IMM12>
,defn<a7_cs, STR("msr")   , OP<0x360'f000>      , FMT_X, CPSR, IMM12>
,defn<a7_cs, STR("msr")   , OP<0x360'f000>      , FMT_X, SPSR, IMM12>


// A5.2.12 Miscellaneous instructions
// XXX code for MRS/MSR incomplete
, defn<a7_cs, STR("mrs") , OP<0x100'0200, void>, FMT_X, REG, REG>
, defn<a7_cs, STR("msr") , OP<0x102'0200, void>, FMT_X, REG, REG>

, defn<a7_cs, STR("bx")  , OP<0x12f'ff10, void>, FMT_X, REG>
, defn<a7_cs, STR("clz") , OP<0x16f'0f10, void>, FMT_X, REG, REG>
, defn<a7_cs, STR("bxj") , OP<0x12f'ff20, void>, FMT_X, REG>
, defn<a7_cs, STR("blx") , OP<0x12f'ff30, void>, FMT_X, REG>
, defn<a7_cs, STR("eret"), OP<0x160'006e, void>>
, defn<a7_cs, STR("bkpt"), OP<0x120'0070, void>, FMT_X, IMM16>
, defn<a7_cs, STR("hpc") , OP<0x140'0070, void>, FMT_X, IMM16>
, defn<a7_cs, STR("smc") , OP<0x160'0070, void>, FMT_X, IMM4>
// NB: alias for SMC
, defn<a7_cs, STR("smi") , OP<0x160'0070, void>, FMT_X, IMM4>

>;

using arm_insn_load_store_l = list<list<>
// A5.3 Load/store word and unsigned byte
>;
using arm_insn_media_l      = list<list<>
// A5.4 Media instructions
>;

// `branch_l` metafunctions
template <unsigned PU, typename NAME, typename...SYNs>
using stm = defn<a7_cs, NAME, OP<0x150'0000 | (PU << 23)>, FMT_X, REGSET>;

template <unsigned PU, typename NAME, typename...SYNs>
using ldm = defn<a7_cs, NAME, OP<0x150'0000 | (PU << 23)>, FMT_X, REGSET>;

using arm_insn_branch_l     = list<list<>
// A5.5 Branch, Branch with Link, block transfer
, defn<a7_cs, STR("stmda"), OP<0x800'0000>, FMT_X, REG       , REGSET>
, defn<a7_cs, STR("stmda"), OP<0x820'0000>, FMT_X, REG_UPDATE, REGSET>

, defn<a7_cs, STR("ldmda"), OP<0x810'0000>, FMT_X, REG       , REGSET>
, defn<a7_cs, STR("ldmda"), OP<0x830'0000>, FMT_X, REG_UPDATE, REGSET>

, defn<a7_cs, STR("stm")  , OP<0x880'0000>, FMT_X, REG       , REGSET>
, defn<a7_cs, STR("stm")  , OP<0x8a0'0000>, FMT_X, REG_UPDATE, REGSET>

, defn<a7_cs, STR("ldm")  , OP<0x880'0000>, FMT_X, REG       , REGSET>
, defn<a7_cs, STR("ldm")  , OP<0x8a0'0000>, FMT_X, REG_UPDATE, REGSET>

, defn<a7_cs, STR("stmdb"), OP<0x920'0000>, FMT_X, REG       , REGSET>
, defn<a7_cs, STR("stmdb"), OP<0x920'0000>, FMT_X, REG_UPDATE, REGSET>

, defn<a7_cs, STR("ldmdb"), OP<0x910'0000>, FMT_X, REG       , REGSET>
, defn<a7_cs, STR("ldmdb"), OP<0x930'0000>, FMT_X, REG_UPDATE, REGSET>

, defn<a7_cs, STR("stmib"), OP<0x980'0000>, FMT_X, REG       , REGSET>
, defn<a7_cs, STR("stmib"), OP<0x9a0'0000>, FMT_X, REG_UPDATE, REGSET>

, defn<a7_cs, STR("ldmib"), OP<0x990'0000>, FMT_X, REG       , REGSET>
, defn<a7_cs, STR("ldmib"), OP<0x9b0'0000>, FMT_X, REG_UPDATE, REGSET>

, stm<0, STR("da"), STR("ed")>  // decrement after,  empty descending
, stm<2, STR("db"), STR("fd")>  // decrement before, full descending
, stm<1, STR("ia"), STR("ea")>  // increment after,  empty ascending
, stm<3, STR("ib"), STR("fa")>  // increment before, full ascending

, ldm<0, STR("da"), STR("fa")>  // decrement after,  full ascending
, ldm<2, STR("db"), STR("ea")>  // decrement before, empty ascending
, ldm<1, STR("ia"), STR("fd")>  // increment after,  full descending 
, ldm<3, STR("ib"), STR("ed")>  // increment before, empty descending

// XXX need to add w/ optional Write-back with PC is register
//, ldm<0, STR("da"), STR("fa")>  // decrement after,  full ascending
//, ldm<2, STR("db"), STR("ea")>  // decrement before, empty ascending
//, ldm<1, STR("ia"), STR("fd")>  // increment after,  full descending 
//, ldm<3, STR("ib"), STR("ed")>  // increment before, empty descending

, defn<a7_cs, STR("b")  , OP<(0xa  << 24)>, FMT_X, IMM24>
, defn<a7_cs, STR("bl") , OP<(0xb  << 24)>, FMT_X, IMM24>
// XXX don't understand `H` bit in `blx` (half-word)
, defn<void, STR("blx"), OP<(0xfau << 24)>, FMT_X, IMM24>



// STMED is pseudonym for STMDA
// LDMFA is pseudonym for LDMDA
// LDMEA is pseudonym for LDMDB
// STMIA, STMEA are pseudonyms for STM
// LDMIA, LDMEA are pseudonyms for LDM
>;

using arm_insn_cp_supv_l    = list<list<>
// A5.6 Coprocessor instructions, and supervisor call
 >;
 
using arm_insn_uncond_l     = list<list<>
// A5.7 Unconditional Instructions
// XXX aliases da, db, ia, ib
, defn<a7_q, STR("srsda")  , OP<0xf84d'0500>, FMT_X, SP, IMM5>
, defn<a7_q, STR("srsda")  , OP<0xf86d'0500>, FMT_X, SP_UPDATE, IMM5>

, defn<a7_q, STR("srsdb")  , OP<0xf84d'0500>, FMT_X, SP, IMM5>
, defn<a7_q, STR("srsdb")  , OP<0xf86d'0500>, FMT_X, SP_UPDATE, IMM5>

, defn<a7_q, STR("srsia")  , OP<0xf84d'0500>, FMT_X, SP, IMM5>
, defn<a7_q, STR("srsia")  , OP<0xf86d'0500>, FMT_X, SP_UPDATE, IMM5>

, defn<a7_q, STR("srsib")  , OP<0xf8cd'0500>, FMT_X, SP, IMM5>
, defn<a7_q, STR("srsib")  , OP<0xf8ed'0500>, FMT_X, SP_UPDATE, IMM5>

// XXX aliases da, db, ia, ib
, defn<a7_q, STR("rfeda")  , OP<0xf84d'0500>, FMT_X, REG>
, defn<a7_q, STR("rfeda")  , OP<0xf86d'0500>, FMT_X, REG_UPDATE>

, defn<a7_q, STR("rfedb")  , OP<0xf84d'0500>, FMT_X, REG>
, defn<a7_q, STR("rfedb")  , OP<0xf86d'0500>, FMT_X, REG_UPDATE>

, defn<a7_q, STR("rfeia")  , OP<0xf84d'0500>, FMT_X, REG>
, defn<a7_q, STR("rfeia")  , OP<0xf86d'0500>, FMT_X, REG_UPDATE>

, defn<a7_q, STR("rfeib")  , OP<0xf8cd'0500>, FMT_X, REG>
, defn<a7_q, STR("rfeib")  , OP<0xf8ed'0500>, FMT_X, REG_UPDATE>

, defn<a7_c, STR("bl")    , OP<0x0b00'0000>, FMT_X, LABEL>
, defn<a7_q , STR("blx")   , OP<0xfa00'0000>, FMT_X, LABEL>

// XXX Co-processor instructions
//, defn<a7_c, STR("stc")   , OP<0x0c00'0000>, FMT_X, COPROC, CP_REG, REG_INDIR>
//, defn<a7_c, STR("stc")   , OP<0x0c00'0000>, FMT_X, COPROC, CP_REG, REG_UPDATE>


// A5.7.1 Memory hints, and miscellaneous
, defn<a7_q, STR("cpsie"), OP<0xf>, FMT_X, IFLAGS>
, defn<a7_q, STR("cpsie"), OP<0xf>, FMT_X, IFLAGS, IMM4>
, defn<a7_q, STR("cpsid"), OP<0xf>, FMT_X, IFLAGS>
, defn<a7_q, STR("cpsid"), OP<0xf>, FMT_X, IFLAGS, IMM4>
, defn<a7_q, STR("cps")  , OP<0xf>, FMT_X, IMM4>

, defn<a7_q, STR("setend"), OP<0xf101'0000>, FMT_X, ENDIAN>
, defn<a7_q, STR("pli")   , OP<0xf450'f000>, FMT_X, REG , IMM12>    // Indirect
, defn<a7_q, STR("pli")   , OP<0xf450'f000>, FMT_X, LABEL>
, defn<a7_q, STR("pli")   , OP<0xf450'f000>, FMT_X, PC, ONES>       // Indirect
, defn<a7_q, STR("pld")   , OP<0xf550'f000>, FMT_X, REG, IMM12>     // Indirect
, defn<a7_q, STR("pldw")  , OP<0xf510'f000>, FMT_X, REG, IMM12>     // Indirect, write-back
, defn<a7_q, STR("pld")   , OP<0xf55f'f000>, FMT_X, LABEL>          // Indirect
, defn<a7_q, STR("pldw")  , OP<0xf55f'f000>, FMT_X, PC, ONES>       // Indirect
, defn<a7_q, STR("clrex") , OP<0xf57f'f01f>>
, defn<a7_q, STR("dsb")   , OP<0xf57f'f040>, FMT_X, DMB_OPTION>
, defn<a7_q, STR("dmb")   , OP<0xf57f'f050>, FMT_X, DMB_OPTION>
, defn<a7_q, STR("isb")   , OP<0xf57f'f060>, FMT_X, ISB_OPTION>
, defn<a7_q, STR("pli")   , OP<0xf650'f000>, FMT_X, REG_INDIR>
, defn<a7_q, STR("pld")   , OP<0xf750'f000>, FMT_X, REG_INDIR>
, defn<a7_q, STR("pldw")  , OP<0xf710'f000>, FMT_X, REG_INDIR>
>;
#endif
#undef STR

using arm_gen_v =
             list<list<>
                 , arm_insn_list_l
//                 , arm_insn_common_l        // prefered mappings: eg ld <reg>, #0 -> clr <reg>
                 , arm_insn_data_l          // A5.2: data insns 
                 , arm_insn_load_store_l    // A5.3: load/store 
                 , arm_insn_branch_l        // A5.5: branch & related
#if 0
                 , arm_insn_media_l         // A5.4: media insns
                 , arm_insn_cp_supv_l       // A5.6: co-processor and supervisor
                 , arm_insn_uncond_l        // A5.7: unconditional insns
#endif
                 >;
}

namespace kas::arm::opc
{
    template <> struct arm_insn_defn_list<OP_ARM_GEN> : gen::arm_gen_v {};
}

#endif

