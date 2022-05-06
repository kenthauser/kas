#ifndef KAS_M68K_M68851_DEFNS_H
#define KAS_M68K_M68851_DEFNS_H

// Declare MMU instructions
//
// XXX XXX
// There are three distinct versions of floating point hardware support
//
// Original is mc68881/82 coprocessors: all instructions and data formats
//     supported in hardware.
//
// Second is found on '040+ hardware. Transcendential functions, constant ROM
//     (and packed data formats) performed by software package
//
// Third  is found in coldfire processors: Even more functions unimplemented
//     in hardware (and extended precession support removed)
//
// All versions (except original co-processor) have `rounding` formats
// for many operations.
//
// Additionally, the coldfire processors don't support `immediate` addressing
// and some `fmovem` formats.
//
// To fully support these variants, four runtime flags are used to identify
// instruction set allowed:
//
//  bool has_fp;
//  bool has_fp_rounding;
//  bool has_fp_exended;
//  bool has_fp_packed;
//
//  `has_fp_*` are cleared if `has_fp` is clear.
//
// opcode selection functions test these runtime flags as follows:
//
// Common subset        : op_fp   -> has_fp & !has_fp_extended & !has_rounding
// Common + CF Rounding : fp_cf   -> has_fp & !has_fp_extended
// CoProcessor + 040HW  : fp_m68k -> has_fp_extended
// 040HW (Rounding)     : fp_rnd  -> has_fp_extended & has_rounding
// CoProcessor          : fp_68881 -> has_fp_packed
// Full (add emulation) : fp_full -> has_fp
//
// Filtering out unsupported data types & unsupported addressing modes
// is done in `m68k_arg_validate.h`
// 
// XXX XXX

#include "m68k_insn_common.h"


namespace kas::m68k::opc::mmu
{
// XXX why is `opc` needed?? Shouldn't enclosed namespaces be searched???
using namespace opc;
using namespace meta;
using namespace hw;

#define STR KAS_STRING

// all coprocessor instructions are F-line instructions
// 040+ requires mmu coprocessor ID be set to STR("000")
static constexpr uint32_t mmu_cpid   = M68K_CPID_MMU;
static constexpr uint32_t mmu_insn_w = 0xf000 + (mmu_cpid << 9);
static constexpr uint32_t mmu_insn_l = mmu_insn_w << 16;

// Since coprocessor instructions use `F-line` opcodes & fixed coprocessor id
// bits 31-25 are immutable. These 7 bits can be used as flags
// to op-code generation metafunctions, masked away before combined
// with the fixed msbs.

static constexpr uint32_t mmu_defn_use_short = 1 << 31;

// mask out 7 bits of flags
static constexpr uint32_t mmu_defn_mask      = ((1<<7)-1) << (9 + 16);

// create a cousin of `M68K_OPCODE` to handle opcode value manipulation
// NB: default TST to `hw::mmu`
template <uint32_t OPCODE, typename TST = void, typename INFO_FN = void>
using MOP = OP<!(OPCODE & mmu_defn_use_short) ?  (mmu_insn_l | (OPCODE &~ mmu_defn_mask))
                                              :  (mmu_insn_w | (OPCODE &  0xffff))
               , meta::if_<std::is_void<TST>, hw::mmu, TST>
               , INFO_FN
               >;

// alias for "mmu short format"
static constexpr auto MSH = mmu_defn_use_short;

using mmu_gen_ops = list<list<>
, defn<sz_v, STR("pflusha"), MOP<0x2200, m68851>>
, defn<sz_v, STR("pflush") , MOP<0x3000, m68851>, void, MMU_FC, Q_4BITS>
, defn<sz_v, STR("pflush") , MOP<0x3400, m68851>, void, MMU_FC, Q_4BITS, CONTROL_ALTER>
, defn<sz_v, STR("pflushs"), MOP<0x3200, m68851>, void, MMU_FC, Q_4BITS>
, defn<sz_v, STR("pflushs"), MOP<0x3600, m68851>, void, MMU_FC, Q_4BITS, CONTROL_ALTER>

, defn<sz_v, STR("pflushr"), MOP<0xa000, m68851>, void, MEM>
, defn<sz_v, STR("ploadr") , MOP<0x2100, m68851>, void, MMU_FC, CONTROL_ALTER>
, defn<sz_v, STR("ploadw") , MOP<0x2000, m68851>, void, MMU_FC, CONTROL_ALTER>

// NB: immed values for `pmovew` can be b/w/l/q based on register 
// NB: see `m68k_reg_defn.h` for register width is encoded in value
// NB: parse with each size, with optional size suffix
, defn<sz_vb, STR("pmovew")  , MOP<0x4000, m68851>, void, GEN, MMU_REG>
, defn<sz_vw, STR("pmovew")  , MOP<0x4000, m68851>, void, GEN, MMU_REG>
, defn<sz_vl, STR("pmovew")  , MOP<0x4000, m68851>, void, GEN, MMU_REG>
// XXX should be `sz_vq` ?
, defn<sz_v , STR("pmovew")  , MOP<0x4000, m68851>, void, GEN, MMU_REG>

// Allow optional byte/word/long suffix to be appended
, defn<sz_vb, STR("pmover")  , MOP<0x4100, m68851>, void, MMU_REG, ALTERABLE>
, defn<sz_vw, STR("pmover")  , MOP<0x4100, m68851>, void, MMU_REG, ALTERABLE>
, defn<sz_vl, STR("pmover")  , MOP<0x4100, m68851>, void, MMU_REG, ALTERABLE>
, defn<sz_v , STR("pmover")  , MOP<0x4100, m68851>, void, MMU_REG, ALTERABLE>

// PSAVE/PRESTORE use short (1 word) format
, defn<sz_v, STR("prestore"), MOP<MSH+0x0140, m68851>, void, CONTROL>
, defn<sz_v, STR("prestore"), MOP<MSH+0x0140, m68851>, void, POST_INCR>
, defn<sz_v, STR("psave")   , MOP<MSH+0x0100, m68851>, void, CONTROL>
, defn<sz_v, STR("psave")   , MOP<MSH+0x0100, m68851>, void, PRE_DECR>

, defn<sz_v, STR("ptestr")  , MOP<0x8200, m68851>, void
                                            , MMU_FC, CONTROL_ALTER, Q_3BITS>
, defn<sz_v, STR("ptestr")  , MOP<0x8200, m68851>, void
                                            , MMU_FC, CONTROL_ALTER, Q_3BITS, ADDR_REG>
, defn<sz_v, STR("ptestw")  , MOP<0x8000, m68851>, void
                                            , MMU_FC, CONTROL_ALTER, Q_3BITS>
, defn<sz_v, STR("ptestw")  , MOP<0x8000, m68851>, void
                                            , MMU_FC, CONTROL_ALTER, Q_3BITS, ADDR_REG>

, defn<sz_l, STR("pvalid")  , MOP<0x2800, m68851>, void, MMU_VAL, CONTROL_ALTER>
, defn<sz_l, STR("pvalid")  , MOP<0x2c00, m68851>, void, ADDR_REG, CONTROL_ALTER>

// '030 MMU Instructions
, defn<sz_v, STR("pflusha"), MOP<0x2200, m68851>>
, defn<sz_v, STR("pflush") , MOP<0x3000, m68851>, void, MMU_FC, Q_4BITS>
, defn<sz_v, STR("pflush") , MOP<0x3400, m68851>, void, MMU_FC, Q_4BITS, CONTROL_ALTER>

// NB: immed values for `pmovew` can be b/w/l/q based on register 
// NB: see `m68k_reg_defn.h` for register width is encoded in value
// NB: parse with each size, with optional size suffix
, defn<sz_vb, STR("pmovew")  , MOP<0x4000, m68851>, void, GEN, MMU_REG>
, defn<sz_vw, STR("pmovew")  , MOP<0x4000, m68851>, void, GEN, MMU_REG>
, defn<sz_vl, STR("pmovew")  , MOP<0x4000, m68851>, void, GEN, MMU_REG>
// XXX should be `sz_vq` ?
, defn<sz_v , STR("pmovew")  , MOP<0x4000, m68851>, void, GEN, MMU_REG>

// Allow optional byte/word/long suffix to be appended
, defn<sz_vb, STR("pmover")  , MOP<0x4100, m68851>, void, MMU_REG, ALTERABLE>
, defn<sz_vw, STR("pmover")  , MOP<0x4100, m68851>, void, MMU_REG, ALTERABLE>
, defn<sz_vl, STR("pmover")  , MOP<0x4100, m68851>, void, MMU_REG, ALTERABLE>
, defn<sz_v , STR("pmover")  , MOP<0x4100, m68851>, void, MMU_REG, ALTERABLE>

, defn<sz_v, STR("ptestr")  , MOP<0x8200, m68851>, void
                                            , MMU_FC, CONTROL_ALTER, Q_3BITS>
, defn<sz_v, STR("ptestr")  , MOP<0x8200, m68851>, void
                                            , MMU_FC, CONTROL_ALTER, Q_3BITS, ADDR_REG>
, defn<sz_v, STR("ptestw")  , MOP<0x8000, m68851>, void
                                            , MMU_FC, CONTROL_ALTER, Q_3BITS>
, defn<sz_v, STR("ptestw")  , MOP<0x8000, m68851>, void
                                            , MMU_FC, CONTROL_ALTER, Q_3BITS, ADDR_REG>

// EC030
, defn<sz_v, STR("ptestr")  , MOP<0x8200, m68851>, void
                                            , MMU_FC, CONTROL_ALTER, Q_3BITS>
, defn<sz_v, STR("ptestr")  , MOP<0x8200, m68851>, void
                                            , MMU_FC, CONTROL_ALTER, Q_3BITS, ADDR_REG>
, defn<sz_v, STR("ptestw")  , MOP<0x8000, m68851>, void
                                            , MMU_FC, CONTROL_ALTER, Q_3BITS>
, defn<sz_v, STR("ptestw")  , MOP<0x8000, m68851>, void
                                            , MMU_FC, CONTROL_ALTER, Q_3BITS, ADDR_REG>


// '040 MMU Instructions
// CINV: 040, LC040: NB: CPID == 2, MMU Extensions, can't use `MOP`
, defn<sz_v, STR("cinvl")   , OP<0xf408, m68040>, void, MMU_CACHES, ADDR_INDIR>
, defn<sz_v, STR("cinvp")   , OP<0xf410, m68040>, void, MMU_CACHES, ADDR_INDIR>
, defn<sz_v, STR("cinva")   , OP<0xf418, m68040>, void, MMU_CACHES, ADDR_INDIR>

// 040, LC040
, defn<sz_v, STR("pflush") , MOP<0x3000, m68851>, void, ADDR_INDIR>
, defn<sz_v, STR("pflushn") , MOP<0x3000, m68851>, void, ADDR_INDIR>
, defn<sz_v, STR("pflusha"), MOP<0x2200, m68851>>
, defn<sz_v, STR("pflushan"), MOP<0x2200, m68851>>

// 040, LC040
, defn<sz_v, STR("ptestr")  , MOP<0x8200, m68851>, void
                                            , MMU_FC, CONTROL_ALTER, Q_3BITS>
, defn<sz_v, STR("ptestr")  , MOP<0x8200, m68851>, void
                                            , MMU_FC, CONTROL_ALTER, Q_3BITS, ADDR_REG>
, defn<sz_v, STR("ptestw")  , MOP<0x8000, m68851>, void
                                            , MMU_FC, CONTROL_ALTER, Q_3BITS>
, defn<sz_v, STR("ptestw")  , MOP<0x8000, m68851>, void
                                            , MMU_FC, CONTROL_ALTER, Q_3BITS, ADDR_REG>

// EC040
, defn<sz_v, STR("pflush") , MOP<0x3000, m68851>, void, ADDR_INDIR>
, defn<sz_v, STR("pflushn") , MOP<0x3000, m68851>, void, ADDR_INDIR>

>;

// store mmu condition code names & values in a type
template <int N, typename NAME>
struct mmu_cc_trait
{
    using type = mmu_cc_trait;
    using code = std::integral_constant<int, N>;
    using name = NAME;
};

// enumerate the MMU condition codes
using mmu_cc_all = list<
      mmu_cc_trait< 0, STR("bs")>
    , mmu_cc_trait< 1, STR("bc")>
    , mmu_cc_trait< 2, STR("ls")>
    , mmu_cc_trait< 3, STR("lc")>
    , mmu_cc_trait< 4, STR("ss")>
    , mmu_cc_trait< 5, STR("sc")>
    , mmu_cc_trait< 6, STR("as")>
    , mmu_cc_trait< 7, STR("ac")>
    , mmu_cc_trait< 8, STR("ws")>
    , mmu_cc_trait< 9, STR("wc")>
    , mmu_cc_trait<10, STR("is")>
    , mmu_cc_trait<11, STR("ic")>
    , mmu_cc_trait<12, STR("gs")>
    , mmu_cc_trait<13, STR("gc")>
    , mmu_cc_trait<14, STR("cs")>
    , mmu_cc_trait<15, STR("cc")>
    >;

namespace detail
{
    // TST defaults to `hw::mmu` via `MOP`
    template <uint32_t OpCode, typename SZ, typename NAME
            , typename TST = void, typename...Args>
    struct apply_mmu_cc
    {
        template <typename CC>
        using invoke = defn<
                          SZ
                        , string::str_cat<NAME, typename CC::name>
                        , MOP<OpCode + CC::code::value, TST>
                        , Args...
                        >;
    };
}

template <uint32_t OpCode, typename...Args>
using mmu_cc = transform<mmu_cc_all, detail::apply_mmu_cc<OpCode, Args...>>;

// branch. minimum is W
using BRANCH_DEL_W = BRANCH_DEL;

using mmu_cc_ops = list<list<>
#if 0
, mmu_cc<MSH+0x80   , sz_v , STR("pb"), m68851, void, BRANCH_DEL_W>
, mmu_cc<0x48'0000  , sz_vw, STR("pdb"), m68851, void, DATA_REG, BRANCH>
, mmu_cc<0x40'0000  , sz_b , STR("ps"), void, void, DATA_ALTER>
, mmu_cc<0x7c'0000  , sz_v , STR("ptrap")>
, mmu_cc<0x7a'0000  , sz_w , STR("ptrap"), void, void, IMMED>
, mmu_cc<0x7b'0000  , sz_l , STR("ptrap"), void, void, IMMED>
#else
, defn <sz_v , STR("pb") , MOP<MSH+0x80 , m68851,  INFO_CCODE_FP1>
                                , FMT_CP_BRANCH, BRANCH_DEL_W>
, defn <sz_vw, STR("pdb"), MOP<0x48'0000, m68851,  INFO_CCODE_FP1>
                                , FMT_CP_BRANCH, DATA_REG, BRANCH>
, defn <sz_b , STR("ps") , MOP<0x40'0000, void, INFO_CCODE_FP1>
                                , FMT_0RM, DATA_ALTER>
, defn <sz_v , STR("ptrap"), MOP<0x7c'0000, void, INFO_CCODE_FP1> >
, defn <sz_w , STR("ptrap"), MOP<0x7a'0000, void, INFO_CCODE_FP1>, FMT_X, IMMED>
, defn <sz_l , STR("ptrap"), MOP<0x7b'0000, void, INFO_CCODE_FP1>, FMT_X, IMMED>
#endif
>;

using mmu_ops_v = list<mmu_gen_ops, mmu_cc_ops>;
}

#undef STR

namespace kas::m68k::opc
{
    template <> struct m68k_insn_defn_list<OP_M68K_MMU> : mmu::mmu_ops_v {};
}

#endif
