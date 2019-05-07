#ifndef KAS_ARM_INSNS_ARM_H
#define KAS_ARM_INSNS_ARM_H
//
// Define the ARM instructions. 
//
// For information in the table format, see `target/tgt_mcode_defn_trait.h`
//
// The format names (and naming convention) are in `arm_format_defn.h`
//
// The argument validators are in `arm_validate.h`
 //
// The common validators with ambiguous names are:
//
// REG          : allow general register
//
// Conventions is for formater type names to list "shifts" for args in order. 
// shift of `X` indicates arg is not inserted in machine code
// for shift if `X`, arg is either determined by "validator" (eg SP) or immediate
// immediate arg validators can determine emit size


#include "arm_insn_common.h"

namespace kas::arm::opc::gen
{
using namespace meta;

#define STR KAS_STRING

using sz_s = sz_w;


///////////////////////////////////////////////////////////////////////////////
//
// Support for instructions which use condition codes
//
#if 0
template <int value, typename NAME>
struct cc_trait
{
    using name = NAME;
    using code = std::integral_constant<int, value>;
};
#endif

using cc_names = list<
          cc_trait< 0, STR("eq")>
        , cc_trait< 1, STR("ne")>
        , cc_trait< 2, STR("cs")>
        , cc_trait< 2, STR("hs")>
        , cc_trait< 3, STR("cc")> 
        , cc_trait< 3, STR("lo")>
        , cc_trait< 4, STR("mi")>
        , cc_trait< 5, STR("pl")>
        , cc_trait< 6, STR("vs")>
        , cc_trait< 7, STR("vc")>
        , cc_trait< 8, STR("hi")>
        , cc_trait< 9, STR("ls")>
        , cc_trait<10, STR("ge")>
        , cc_trait<11, STR("lt")>
        , cc_trait<12, STR("lt")>
        , cc_trait<13, STR("le")>
        , cc_trait<14, STR("")>
        , cc_trait<14, STR("al")>
        >;
#if 0
// a `callable` to create `insn` for each condition code
template <unsigned shift, typename SZ, typename NAME, typename...Args>
struct cc_gen_defn
{
    template <typename CC>
    using invoke = defn<SZ
                      , string::str_cat<NAME, typename CC::name>
                      , Args...
                      >;
};
#endif

// condition-codes are 4 msbs of INSN
#if 0
template <typename...Ts>
using cond = transform<cc_names, cc_gen_defn<32-4, Ts...>>;
#else
template <typename...Ts>
using cond = defn<Ts...>;
#endif

template <typename...Ts>
using Xcond = transform<cc_names, cc_gen_defn<32-4, Ts...>>;


using arm_insn_common_l = list<list<>

// Dummy machine-code for "list" opcode
, defn<sz_w, STR("*LIST*"), OP<0>, FMT_LIST, REG, REG>

// "override" special cases to prefer different encodings
// classic case is "ld reg, #0" -> mapped to "clr <reg>"

// XXX SUB/ADD with PC: form PC-relative address

// assemble `stm sp!, ...` as push
, cond<sz_w, STR("stm") , OP<0x92d'0000>, FMT_X, SP_UPDATE, REGSET>
// assemble `ldm sp!, ...` as pop
, cond<sz_w, STR("ldm") , OP<0x8ad'0000>, FMT_X, SP_UPDATE, REGSET>

>;

// NB: base code for register data-processing INSNs is zero.
//     base code for immediate versions is 0x200'0000
template <typename SZ, typename NAME, unsigned ARM_OP, unsigned ARM_OP2 = 0, typename IMM5 = void>
using data_processing = list<list<>
// register, (4 formats)
, cond<SZ, NAME, OP<(ARM_OP << 20) | (ARM_OP2 << 5)>
                , FMT_X, REG, REG, REG, SHIFT> 
, cond<SZ, NAME, OP<(ARM_OP << 20) | (ARM_OP2 << 5)>
                , FMT_X, REG, REG, REG> 
, cond<SZ, NAME, OP<(ARM_OP << 20) | (ARM_OP2 << 5)>
                , FMT_X, REG, REG, SHIFT> 
, cond<SZ, NAME, OP<(ARM_OP << 20) | (ARM_OP2 << 5)>
                , FMT_X, REG, REG>
// immed (2 formats)
, cond<SZ, NAME, OP<0x200'0000 | (ARM_OP << 20) | (ARM_OP2 << 5)>
                , FMT_X, REG, REG, IMM12>
, cond<SZ, NAME, OP<0x200'0000 | (ARM_OP << 20) | (ARM_OP2 << 5)>
                , FMT_X, REG, REG, IMM12>
>;

template <typename NAME, unsigned ARM_OP>
using saturating_math = list<list<>
// register, (2 formats)
, cond<sz_w, NAME, OP<0x100'0050 | (ARM_OP << 21), void>, FMT_X, REG, REG, REG>
, cond<sz_w, NAME, OP<0x100'0050 | (ARM_OP << 21), void>, FMT_X, REG, REG>
>;

// Data Processing: ARM-7 A+R section 5.2, pg A5-195
using arm_insn_data_l       = list<list<>

// use `meta-function` to generated related instructions (3 sections)
// A5.2.1 Data processing(register)
// A5.2.2 Data processing(register-shifted register)
// A5.2.3 Data processing(register-shifted register)
, data_processing<sz_s, STR("and"),  0>
, data_processing<sz_s, STR("eor"),  2>
, data_processing<sz_s, STR("sub"),  4>
, data_processing<sz_s, STR("rsb"),  6>
, data_processing<sz_s, STR("add"),  8>
, data_processing<sz_s, STR("adc"), 10>
, data_processing<sz_s, STR("sbc"), 12>
, data_processing<sz_s, STR("rsc"), 14>
//, data_processing: op = 0b10xx0  See below
, data_processing<sz_w, STR("tst"), 17>
, data_processing<sz_w, STR("teq"), 19>
, data_processing<sz_w, STR("cmp"), 21>
, data_processing<sz_w, STR("cmn"), 23>
, data_processing<sz_s, STR("orr"), 24>
, data_processing<sz_s, STR("mov"), 26, 0, SHIFT_Z>
// XXX No IMMED for LSL, etc...
, data_processing<sz_s, STR("lsl"), 26, 0, SHIFT_NZ>
, data_processing<sz_s, STR("lsr"), 26, 1>
, data_processing<sz_s, STR("asr"), 26, 2>
, data_processing<sz_s, STR("rrx"), 26, 3, SHIFT_Z>
, data_processing<sz_s, STR("ror"), 26, 3, SHIFT_NZ>
, data_processing<sz_s, STR("bic"), 28>
, data_processing<sz_s, STR("mvn"), 30>

// A5.2.5 Multiply & Multiply Accumulate
, cond<sz_s, STR("mul")  , OP<0x000'0090>      , FMT_X, REG, REG, REG>
, cond<sz_s, STR("mul")  , OP<0x000'0090>      , FMT_X, REG, REG>
, cond<sz_s, STR("mla")  , OP<0x020'0090>      , FMT_X, REG, REG, REG, REG>
, cond<sz_w, STR("umaal"), OP<0x040'0090, void>, FMT_X, REG, REG, REG, REG>
, cond<sz_w, STR("mls")  , OP<0x060'0090, void>, FMT_X, REG, REG, REG, REG>
, cond<sz_w, STR("mls")  , OP<0x060'0090, void>, FMT_X, REG, REG, REG, REG>
, cond<sz_s, STR("umull"), OP<0x080'0090>      , FMT_X, REG, REG, REG, REG>
, cond<sz_s, STR("umlal"), OP<0x0a0'0090>      , FMT_X, REG, REG, REG, REG>
, cond<sz_s, STR("smull"), OP<0x0c0'0090>      , FMT_X, REG, REG, REG, REG>
, cond<sz_s, STR("smlal"), OP<0x0e0'0090>      , FMT_X, REG, REG, REG, REG>
 
// A5.2.6 saturating math for code 0b10xx0 => 19-22
, saturating_math<STR("qadd") , 0>
, saturating_math<STR("qsub") , 1>
, saturating_math<STR("qdadd"), 2>
, saturating_math<STR("qdsub"), 3>

// A5.6.7 Halfword multiply and multiply accumulate
, cond<sz_w, STR("smlabb"), OP<0x100'0080, void>, FMT_X, REG, REG, REG, REG>
, cond<sz_w, STR("smlabt"), OP<0x100'00a0, void>, FMT_X, REG, REG, REG, REG>
, cond<sz_w, STR("smlatb"), OP<0x100'00c0, void>, FMT_X, REG, REG, REG, REG>
, cond<sz_w, STR("smlatt"), OP<0x100'00e0, void>, FMT_X, REG, REG, REG, REG>

, cond<sz_w, STR("smlawb"), OP<0x120'0080, void>, FMT_X, REG, REG, REG, REG>
, cond<sz_w, STR("smlawt"), OP<0x120'00c0, void>, FMT_X, REG, REG, REG, REG>
, cond<sz_w, STR("smulwb"), OP<0x120'00a0, void>, FMT_X, REG, REG, REG>
, cond<sz_w, STR("smulwb"), OP<0x120'00a0, void>, FMT_X, REG, REG, REG, REG>
, cond<sz_w, STR("smulwt"), OP<0x120'00e0, void>, FMT_X, REG, REG, REG>
, cond<sz_w, STR("smulwt"), OP<0x120'00e0, void>, FMT_X, REG, REG, REG, REG>

, cond<sz_w, STR("smlalbb"), OP<0x140'0080, void>, FMT_X, REG, REG, REG, REG>
, cond<sz_w, STR("smlalbt"), OP<0x140'00a0, void>, FMT_X, REG, REG, REG, REG>
, cond<sz_w, STR("smlaltb"), OP<0x140'00c0, void>, FMT_X, REG, REG, REG, REG>
, cond<sz_w, STR("smlaltt"), OP<0x140'00e0, void>, FMT_X, REG, REG, REG, REG>

, cond<sz_w, STR("smulbb"), OP<0x160'0080, void>, FMT_X, REG, REG, REG>
, cond<sz_w, STR("smulbb"), OP<0x160'0080, void>, FMT_X, REG, REG>
, cond<sz_w, STR("smulbt"), OP<0x160'00a0, void>, FMT_X, REG, REG, REG>
, cond<sz_w, STR("smulbt"), OP<0x160'00a0, void>, FMT_X, REG, REG>
, cond<sz_w, STR("smultb"), OP<0x160'00c0, void>, FMT_X, REG, REG, REG>
, cond<sz_w, STR("smultb"), OP<0x160'00c0, void>, FMT_X, REG, REG>
, cond<sz_w, STR("smultt"), OP<0x160'00e0, void>, FMT_X, REG, REG, REG>
, cond<sz_w, STR("smultt"), OP<0x160'00e0, void>, FMT_X, REG, REG>

// A5.2.8 Extra Load/store instructions
// XXX
// A5.2.9 Extra Load/store instructions, unprivileged
// XXX
// A5.2.10 Sychronization primitives
,cond<sz_w, STR("swp")   , OP<0x100'0090>      , FMT_X, REG, REG, REG_INDIR>
,cond<sz_w, STR("swpb")  , OP<0x140'0090>      , FMT_X, REG, REG, REG_INDIR>
,cond<sz_w, STR("strex") , OP<0x180'0090, void>, FMT_X, REG, REG, REG_OFFSET>
,cond<sz_w, STR("ldrex") , OP<0x190'0f9f, void>, FMT_X, REG, REG_OFFSET>
,cond<sz_w, STR("strexd"), OP<0x1a0'0f90, void>, FMT_X, REG, REG, REG, REG_INDIR>
,cond<sz_w, STR("ldrexd"), OP<0x1b0'0f9f, void>, FMT_X, REG, REG, REG_INDIR>
,cond<sz_w, STR("strexb"), OP<0x1c0'0f90, void>, FMT_X, REG, REG, REG_INDIR>
,cond<sz_w, STR("ldrexb"), OP<0x1d0'0f9f, void>, FMT_X, REG, REG_INDIR>
,cond<sz_w, STR("strexh"), OP<0x1c0'0f90, void>, FMT_X, REG, REG, REG_INDIR>
,cond<sz_w, STR("ldrexh"), OP<0x1d0'0f9f, void>, FMT_X, REG, REG_INDIR>

// A5.2.11 MSR (immediate), and hints
,cond<sz_w, STR("nop")   , OP<0x320'f000, void>, FMT_X>
,cond<sz_w, STR("yield") , OP<0x320'f001, void>, FMT_X>
,cond<sz_w, STR("wfe")   , OP<0x320'f002, void>, FMT_X>
,cond<sz_w, STR("wfi")   , OP<0x320'f003, void>, FMT_X>
,cond<sz_w, STR("sev")   , OP<0x320'f004, void>, FMT_X>
,cond<sz_w, STR("csdb")  , OP<0x320'f014, void>, FMT_X>
,cond<sz_w, STR("dbg")   , OP<0x320'f0f0, void>, FMT_X, IMM4>

// XXX need work on inserters
,cond<sz_w, STR("msr")   , OP<0x324'f000>      , FMT_X, APSR, IMM12>
,cond<sz_w, STR("msr")   , OP<0x328'f000>      , FMT_X, CPSR, IMM12>

,cond<sz_w, STR("msr")   , OP<0x320'f000>      , FMT_X, APSR, IMM12>
,cond<sz_w, STR("msr")   , OP<0x320'f000>      , FMT_X, CPSR, IMM12>
,cond<sz_w, STR("msr")   , OP<0x320'f000>      , FMT_X, SPSR, IMM12>

,cond<sz_w, STR("msr")   , OP<0x360'f000>      , FMT_X, APSR, IMM12>
,cond<sz_w, STR("msr")   , OP<0x360'f000>      , FMT_X, CPSR, IMM12>
,cond<sz_w, STR("msr")   , OP<0x360'f000>      , FMT_X, SPSR, IMM12>


// A5.2.12 Miscellaneous instructions
// XXX code for MRS/MSR incomplete
, cond<sz_w, STR("mrs") , OP<0x100'0200, void>, FMT_X, REG, REG>
, cond<sz_w, STR("msr") , OP<0x102'0200, void>, FMT_X, REG, REG>

, cond<sz_w, STR("bx")  , OP<0x12f'ff10, void>, FMT_X, REG>
, cond<sz_w, STR("clz") , OP<0x16f'0f10, void>, FMT_X, REG, REG>
, cond<sz_w, STR("bxj") , OP<0x12f'ff20, void>, FMT_X, REG>
, cond<sz_w, STR("blx") , OP<0x12f'ff30, void>, FMT_X, REG>
, cond<sz_w, STR("eret"), OP<0x160'006e, void>>
, cond<sz_w, STR("bkpt"), OP<0x120'0070, void>, FMT_X, IMM16>
, cond<sz_w, STR("hpc") , OP<0x140'0070, void>, FMT_X, IMM16>
, cond<sz_w, STR("smc") , OP<0x160'0070, void>, FMT_X, IMM4>
// NB: alias for SMC
, cond<sz_w, STR("smi") , OP<0x160'0070, void>, FMT_X, IMM4>

>;

using arm_insn_load_store_l = list<list<>
// A5.3 Load/store word and unsigned byte
>;
using arm_insn_media_l      = list<list<>
// A5.4 Media instructions
>;

// `branch_l` metafunctions
template <unsigned PU, typename NAME, typename...SYNs>
using stm = cond<sz_w, NAME, OP<0x150'0000 | (PU << 23)>, FMT_X, REGSET>;

template <unsigned PU, typename NAME, typename...SYNs>
using ldm = cond<sz_w, NAME, OP<0x150'0000 | (PU << 23)>, FMT_X, REGSET>;

using arm_insn_branch_l     = list<list<>
// A5.5 Branch, Branch with Link, block transfer
, cond<sz_w, STR("stmda"), OP<0x800'0000>, FMT_X, REG       , REGSET>
, cond<sz_w, STR("stmda"), OP<0x820'0000>, FMT_X, REG_UPDATE, REGSET>

, cond<sz_w, STR("ldmda"), OP<0x810'0000>, FMT_X, REG       , REGSET>
, cond<sz_w, STR("ldmda"), OP<0x830'0000>, FMT_X, REG_UPDATE, REGSET>

, cond<sz_w, STR("stm")  , OP<0x880'0000>, FMT_X, REG       , REGSET>
, cond<sz_w, STR("stm")  , OP<0x8a0'0000>, FMT_X, REG_UPDATE, REGSET>

, cond<sz_w, STR("ldm")  , OP<0x880'0000>, FMT_X, REG       , REGSET>
, cond<sz_w, STR("ldm")  , OP<0x8a0'0000>, FMT_X, REG_UPDATE, REGSET>

, cond<sz_w, STR("stmdb"), OP<0x920'0000>, FMT_X, REG       , REGSET>
, cond<sz_w, STR("stmdb"), OP<0x920'0000>, FMT_X, REG_UPDATE, REGSET>

, cond<sz_w, STR("ldmdb"), OP<0x910'0000>, FMT_X, REG       , REGSET>
, cond<sz_w, STR("ldmdb"), OP<0x930'0000>, FMT_X, REG_UPDATE, REGSET>

, cond<sz_w, STR("stmib"), OP<0x980'0000>, FMT_X, REG       , REGSET>
, cond<sz_w, STR("stmib"), OP<0x9a0'0000>, FMT_X, REG_UPDATE, REGSET>

, cond<sz_w, STR("ldmib"), OP<0x990'0000>, FMT_X, REG       , REGSET>
, cond<sz_w, STR("ldmib"), OP<0x9b0'0000>, FMT_X, REG_UPDATE, REGSET>

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

, Xcond<sz_w, STR("b")  , OP<(0xa  << 24)>, FMT_X, IMM24>
, Xcond<sz_w, STR("bl") , OP<(0xb  << 24)>, FMT_X, IMM24>
// XXX don't understand `H` bit in `blx`
, defn<sz_w, STR("blx"), OP<(0xfau << 24)>, FMT_X, IMM24>



// STMED is pseudonym for STMDA
// LDMFA is pseudonym for LDMDA
// STMIA, STMEA are pseudonyms for STM
// LDMIA, LDMEA are pseudonyms for LDM
// LDMIA, LDMEA are pseudonyms for LDM
// LDMEA psudonym for LDMDB


>;
using arm_insn_cp_supv_l    = list<list<>
// A5.6 Coprocessor instructions, and supervisor call
>;
using arm_insn_uncond_l     = list<list<>
// A5.7 Unconditional Instructions


// XXX temp
, cond<sz_w, STR("b")  , OP<0xa00'0000>, FMT_X, LABEL>
, defn<sz_w, STR("mov")   , OP<0x8000>, FMT_X   , REG, REG>
, defn<sz_w, STR("push")  , OP<0x8001>, FMT_X   , REG>

>;


using arm_insn_list =
             list<list<>
                 , arm_insn_common_l        // prefered mappings: eg ld <reg>, #0 -> clr <reg>
                 , arm_insn_data_l          // A5.2: data insns 
                 , arm_insn_load_store_l    // A5.3: load/store 
                 , arm_insn_media_l         // A5.4: media insns
                 , arm_insn_branch_l        // A5.5: branch & related
                 , arm_insn_cp_supv_l       // A5.6: co-processor and supervisor
                 , arm_insn_uncond_l        // A5.7: unconditional insns
                 >;
}

namespace kas::arm::opc
{
    template <> struct arm_insn_defn_list<OP_ARM_GEN> : gen::arm_insn_list {};
}

#undef STR

#endif

