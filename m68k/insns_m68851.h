#ifndef KAS_M68K_M68851_DEFNS_H
#define KAS_M68K_M68851_DEFNS_H

// Declare MMU instructions
//
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


#include "m68k_opcode_formats.h"
#include "m68k_arg_validate.h"
#include "m68k_size_defn.h"
//#include "m68k_insn_select.h"
#include "defns_mmu.h"

#include "kas/kas_string.h"

namespace kas::m68k::opc::mmu
{
// XXX why is `opc` needed?? Shouldn't enclosed namespaces be searched???
using namespace opc;
using namespace meta;
using namespace hw;

#define STR KAS_STRING

// all coprocessor instructions are F-line instructions
// 040 & ColdFire support requires math coprocessor ID be set to STR("001")
static constexpr uint32_t mmu_cpid   = M68K_CPID_MMU;
static constexpr uint32_t mmu_insn_w = 0xf000 + (mmu_cpid << 9);
static constexpr uint32_t mmu_insn_l = mmu_insn_w << 16;
static constexpr uint32_t mmu_insn_reg_mem   = 1 << 14;

// Since coprocessor instructions use `F-line` opcodes & fixed coprocessor id
// bits 31-25 are immutable. These 7 bits can be used as flags
// to op-code generation metafunctions, masked away before combined
// with the fixed msbs.

static constexpr uint32_t mmu_insn_use_short = 1 << 31;
static constexpr uint32_t mmu_insn_rounding  = 1 << 30;
static constexpr uint32_t mmu_insn_bcc       = 1 << 29;
static constexpr uint32_t mmu_insn_dbcc      = 1 << 28;

static constexpr uint32_t mmu_insn_mask      = ((7<<1)-1) << (9 + 16);

// GENERATE M68K Instruction: (as a `meta callable`)
// Use OP, NAME, & Argument list to generate a single insn (from size_trait)
// modify per `mmu_insn_*` flags or'd into OpCode
template <typename OP_T, typename ADD_SIZE, typename TST, typename NAME, typename...Args>
struct make_mmu_insn
{
    static constexpr auto Op = OP_T::value;

    // pre-calculate some variables...
    static constexpr auto opcode = !(Op & mmu_insn_use_short)
                                   ? (mmu_insn_l | (Op &~ mmu_insn_mask))
                                   : (mmu_insn_w | (Op &  0xffff));

    template <typename Size_Trait>
    using invoke = M68K_OP<
                  ADD_SIZE
                , string::str_cat<STR("f"), NAME>
                , Size_Trait
                , M68K_INFO<
                      opcode
                    , M68K_OPC<
                        // pick format based on branch bits
                        (Op & mmu_insn_bcc)  ? M68K_OPC_CP_BRANCH :
                          (Op & mmu_insn_dbcc) ? M68K_OPC_CP_DBCC   :
                          M68K_OPC_NONE

                        // pick is-a test based on rounding
                      , if_c<!!(Op & mmu_insn_rounding), fpu_rnd, fpu_base>

                        // has-a test is forwarded...
                      , TST
                     >
                    , INFO_SIZE_FLT     // place size into insn
                    >
                // FMT & Validations...
                , Args...
                >;
};

// generate a INSN for each of the 7 Floating Point source sizes
// set "Reg/Mem" bit & use general format
template <std::size_t Op, typename...Args>
using mmu_gen_all = transform<
                        sz_all
                      , make_mmu_insn<
                              size_t<Op | mmu_insn_reg_mem>
                            , std::true_type
                            , Args...
                            >
                      >;

// generate a INSN for only the specified source size
template <typename SizeTrait, uint32_t Op, typename...Args>
using mmu_gen_one = transform<SizeTrait, make_mmu_insn<size_t<Op>, std::false_type, Args...>>;

// convenience aliases for specific source sizes
template <uint32_t Op, typename...Args> using mmu_gen_x = mmu_gen_one<sz_x, Op, Args...>;
template <uint32_t Op, typename...Args> using mmu_gen_l = mmu_gen_one<sz_l, Op, Args...>;
template <uint32_t Op, typename...Args> using mmu_gen_d = mmu_gen_one<sz_d, Op, Args...>;
template <uint32_t Op, typename...Args> using mmu_gen_p = mmu_gen_one<sz_p, Op, Args...>;
template <uint32_t Op, typename...Args> using mmu_gen_v = mmu_gen_one<sz_void, Op, Args...>;

// These metafunctions generate the single and dual argument fp instructions
// are split into `interface` and `helper` routines.
// This separation is required to support generation of "040 rounding" instructions
namespace detail {
    template <int Op, typename...Args>
    using mmu_gen_op_all = mmu_gen_all<Op, Args...>;

    template <int Op, typename TST, typename...Args>
    using mmu_gen_op_x   = list<list<>
                , mmu_gen_x <Op, TST, Args...>
                , mmu_gen_d <Op, TST, Args...>
                >;

    template <int N, typename TST, typename NAME>
    using mmu_two = list<list<>
        , mmu_gen_op_all <(N &~ 0x80), TST, NAME, FMT_0RM_7FP, DATA, FP_REG>
        , mmu_gen_op_x   <(N &~ 0x80), TST, NAME, FMT_10FP_7FP, FP_REG, FP_REG>
        >;

    template <int N, typename TST, typename NAME>
    using mmu_one = list<list<>
        , mmu_two <(N &~ 0x80), TST, NAME>
        , mmu_gen_op_x<(N &~ 0x80), TST, NAME, FMT_DUPL_FP, FP_REG>
        >;
}

template <int N, typename TST, typename NAME>
using mmu_two = list<list<>
        , detail::mmu_two<(N &~ 0x80), TST, NAME>
        , if_c<!(N & 0x80), list<>,
                // add in rounding formats
                list<list<>
                    , detail::mmu_two<((N &~ 0x80) | 0x40 | mmu_insn_rounding)
                                    , TST, string::str_cat<STR("s"), NAME>>
                    , detail::mmu_two<((N &~ 0x80) | 0x44 | mmu_insn_rounding)
                                    , TST, string::str_cat<STR("d"), NAME>>
                >
            >
        >;

template <int N, typename TST, typename NAME>
using mmu_one = list<list<>
        , detail::mmu_one<(N &~ 0x80), TST, NAME>
        , if_c<!(N & 0x80), list<>,
                // add in rounding formats
                list<list<>
                    , detail::mmu_one<((N &~ 0x80) | 0x40 | mmu_insn_rounding)
                                    , TST, string::str_cat<STR("s"), NAME>>
                    , detail::mmu_one<((N &~ 0x80) | 0x44 | mmu_insn_rounding)
                                    , TST, string::str_cat<STR("d"), NAME>>
                >
            >
        >;

// ftst doesn't support rounding -- so just implement
template <int N, typename TST, typename NAME>
using mmu_tst = list<list<>
        , detail::mmu_gen_op_all<(N &~ 0x80), TST, NAME, FMT_0RM, DATA>
        , detail::mmu_gen_op_x  <(N &~ 0x80), TST, NAME, FMT_10FP, FP_REG>
        >;


// for `monadic` & `dyadic` instructions (*only* mmu_one & mmu_two):
// op-mode is 7 lsb of second word.
// op-mode 0x80  bit indicates rounding modes must also be generated
// NB: all cpu implementations but no coprocessors support rounding...

// NB: each item generates between 8 and 27 distinct instructions

using mmu_gen_ops = list<list<>
    , v<STR("cinvl"), OP<0xf200>, CACHES, ADDR_INDIR>
    , v<STR("cinvp"), OP<0xf200>, CACHES, ADDR_INDIR>
    , v<STR("cinva"), OP<0xf200>, CACHES>
    , v<STR("cpushl"), OP<0xf220>, CACHES, ADDR_INDIR>
    , v<STR("cpushp"), OP<0xf220>, CACHES, ADDR_INDIR>
    , v<STR("cpusha"), OP<0xf220>, CACHES>
    // 030
    , v<STR("pflusha"), OP<0xf000>>
    , v<STR("pflush"), FC, MASK>
    , v<STR("pflush"), FC, MASK, CONTROL_ALTER>
    // 040, LC040
    , v<STR("pflush"), OP<0xf000>, ADDR_INDIR>
    , v<STR("pflushn"), OP<0xf000>, ADDR_INDIR>
    , v<STR("pflusha"), OP<0xf000>>
    , v<STR("pflushan"), OP<0xf000>>

    // EC040
    , v<STR("pflush"), OP<0xf000>, ADDR_INDIR>
    , v<STR("pflushn"), OP<0xf000>, ADDR_INDIR>
    // 68851
    , v<STR("pflusha"), OP<0xf000>>
    , v<STR("pflush"),  OP<0xf000> FC, MASK>
    , v<STR("pflushs"),  OP<0xf000> FC, MASK>
    , v<STR("pflush"),  OP<0xf000> FC, MASK, CONTROL_ALTER>
    , v<STR("pflushs"),  OP<0xf000> FC, MASK, CONTROL_ALTER>
    , v<STR("pflushr"),  OP<0xf000>, MEMORY>
    // 030 (only), 68551
    , v<STR("ploadr"), OP<>, FC, CONTROL_ALTER>
    , v<STR("ploadw"), OP<>, FC, CONTROL_ALTER>
    // 060 only
    , v<STR("plpar"), OP<0xf5c8>, ADDR_INDIR>
    , v<STR("plpaw"), OP<0xf588>, ADDR_INDIR>
    // 030(only)
    , lwQ<STR("pmove"), OP<>,  MRn, CONTROL_ALTER>
    , lwQ<STR("pmove"), OP<>,  CONTROL_ALTER, MRn, CONTROL_ALTER>
    , lwQ<STR("pmovefd"), OP<>,CONTROL_ALTER, MRn>
    // EC030
    , lwQ<STR("pmove"), OP<>,  MRn, CONTROL_ALTER>
    , lwQ<STR("pmove"), OP<>,  CONTROL_ALTER, MRn, CONTROL_ALTER>
    // 68551
    , lwbQ<STR("pmove"), OP<>,  MRn, CONTROL_ALTER>
    , lwbQ<STR("pmove"), OP<>,  GEN, MRn, CONTROL_ALTER>

    // 68551
    , v<STR("prestore"), OP<0xf240>, CONTROL>
    , v<STR("prestore"), OP<0xf240>, POST_INCR>
    , v<STR("psave"), OP<0xf240>, CONTROL>
    , v<STR("psave"), OP<0xf240>, POST_INCR>

    // 030 only
    , v<STR("ptestr"), OP<>, FC, CONTROL_ALTER, Q_3BITS>
    , v<STR("ptestr"), OP<>, FC, CONTROL_ALTER, Q_3BITS, ADDR_REG>
    , v<STR("ptestw"), OP<>, FC, CONTROL_ALTER, Q_3BITS>
    , v<STR("ptestw"), OP<>, FC, CONTROL_ALTER, Q_3BITS, ADDR_REG>
    // EC030
    , v<STR("ptestr"), OP<>, FC, CONTROL_ALTER>
    , v<STR("ptestw"), OP<>, FC, CONTROL_ALTER>
    // 040, LC040, (not 060)
    , v<STR("ptestr"), OP<>, ADDR_INDIR>
    , v<STR("ptestw"), OP<>, ADDR_INDIR>
    // 68551
    , v<STR("ptestr"), OP<>, FC, CONTROL_ALTER, Q_3BITS, ADDR_INDIR>
    , v<STR("ptestw"), OP<>, FC, CONTROL_ALTER, Q_3BITS, ADDR_INDIR>

    // 68551
    , l<STR("pvalid"), OP<>, VAL, CONTROL_ALTER>
    , l<STR("pvalid"), OP<>, An, CONTROL_ALTER>

    // coldfire: CPID == 2
    , v<STR<"cpushl">, OP<0xf268, coldfire>, FMT_X_0A, CF_DC, ADDR_INDIR>
    , v<STR<"cpushl">, OP<0xf2a8, coldfire>, FMT_X_0A, CF_IC, ADDR_INDIR>
    , v<STR<"cpushl">, OP<0xf2e8, coldfire>, FMT_X_0A, CF_BC, ADDR_INDIR>

// 68551
 //   PBcc PDBcc PScc PTRAPcc, PTRAPcc.W, PTRAPcc.L


using fpu_gen_ops = list<list<>
// opcodes using "regular" opcode formats
    , mmu_two<0x80, fpu_basic, STR("move")>
    , mmu_one<0x01, fpu_intrz, STR("int")>
    , mmu_one<0x02, fpu_trig,  STR("sinh")>
    , mmu_one<0x03, fpu_intrz, STR("intrz")>
    , mmu_one<0x84, fpu_basic, STR("sqrt")>
    , mmu_one<0x06, fpu_trig,  STR("lognp1")>
    , mmu_one<0x08, fpu_trig,  STR("etoxm1")>
    , mmu_one<0x09, fpu_trig,  STR("tanh")>
    , mmu_one<0x0a, fpu_trig,  STR("atan")>
    , mmu_one<0x0c, fpu_trig,  STR("asin")>
    , mmu_one<0x0d, fpu_trig,  STR("atanh")>
    , mmu_one<0x0e, fpu_trig,  STR("sin")>
    , mmu_one<0x0f, fpu_trig,  STR("tan")>
    , mmu_one<0x10, fpu_trig,  STR("etox")>
    , mmu_one<0x11, fpu_trig,  STR("twotox")>
    , mmu_one<0x12, fpu_trig,  STR("tentox")>
    , mmu_one<0x14, fpu_trig,  STR("logn")>
    , mmu_one<0x15, fpu_trig,  STR("log10")>
    , mmu_one<0x16, fpu_trig,  STR("log2")>
    , mmu_one<0x98, fpu_basic, STR("abs")>
    , mmu_one<0x19, fpu_trig,  STR("cosh")>
    , mmu_one<0x9a, fpu_basic, STR("neg")>
    , mmu_one<0x1c, fpu_trig,  STR("acos")>
    , mmu_one<0x1d, fpu_trig,  STR("cos")>
    , mmu_one<0x1e, fpu_trig,  STR("getexp")>
    , mmu_one<0x1f, fpu_trig,  STR("getman")>
    , mmu_two<0xa0, fpu_basic, STR("div")>
    , mmu_two<0x21, fpu_m68k,  STR("mod")>
    , mmu_two<0xa2, fpu_basic, STR("add")>
    , mmu_two<0xa3, fpu_basic, STR("mul")>
    , mmu_two<0x24, fpu_m68k,  STR("sgldiv")>
    , mmu_two<0x25, fpu_m68k,  STR("rem")>
    , mmu_two<0x26, fpu_trig,  STR("scale")>
    , mmu_two<0x27, fpu_m68k,  STR("sglmul")>
    , mmu_two<0xa8, fpu_basic, STR("sub")>
    , mmu_two<0x38, fpu_basic, STR("cmp")>
    , mmu_tst<0x3a, fpu_m68k,  STR("tst")>

// sincos is special (three arguments)
    , mmu_gen_all<0x30, fpu_trig
                , STR("sincos"), FMT_0RM_0FP_7FP, DATA, FP_REG, FP_REG>
    , mmu_gen_x  <0x30, fpu_trig
                , STR("sincos"), FMT_10FP_0FP_7FP, FP_REG, FP_REG, FP_REG>

// fmove special instructions:: kfactor & constant ROM
    , mmu_gen_x <0x5c00, fpu_trig
                , STR("movecr"), FMT_CR0_7FP, Q_7BITS, FP_REG>
    // XXX don't know assembler format of "fortran" insns
    // , mmu_gen_p  <0x00, STR("move"), FP_REG, DATA, KF_DYNAMIC>
    // , mmu_gen_p  <0x00, STR("move"), FP_REG, DATA, KF_STATIC>

// fmove: fp control registers
    , mmu_gen_l <0x8000, fpu_basic, STR("move"), FMT_0RM_10FC, DATA, FCTRL_REG>
    , mmu_gen_l <0xa000, fpu_basic, STR("move"), FMT_10FC_0RM, FCTRL_REG, DATA_ALTER>

// fmove: FPIAR can also use address registers
    , mmu_gen_l <0x8000, fpu_basic, STR("move"), FMT_0RM_10FC, ADDR_REG, FPIAR>
    , mmu_gen_l <0xa000, fpu_basic, STR("move"), FMT_10FC_0RM, FPIAR, ADDR_REG>

// fsave/frestore
    , mmu_gen_v <0x100 | mmu_insn_use_short, fpu_basic
                , STR("save"), FMT_0RM, CONTROL_ALTER>
    , mmu_gen_v <0x100 | mmu_insn_use_short, fpu_m68k
                , STR("save"), FMT_0RM, PRE_DECR>
    , mmu_gen_v <0x140 | mmu_insn_use_short, fpu_m68k
                , STR("restore"), FMT_0RM, CONTROL>
    , mmu_gen_v <0x140 | mmu_insn_use_short, fpu_m68k
                , STR("restore"), FMT_0RM, POST_INCR>

// fmovem: fp registers: static list & dynamic (via DATA REGISTER) versions
    , mmu_gen_x <0xf000, fpu_m68k, STR("movem"), FMT_I8_0RM, FP_REGSET, CONTROL_ALTER>
    , mmu_gen_x <0xe000, fpu_m68k, STR("movem"), FMT_I8R_0RM, FP_REGSET, PRE_DECR>
    , mmu_gen_x <0xd000, fpu_m68k, STR("movem"), FMT_0RM_I8, CONTROL, FP_REGSET>
    , mmu_gen_x <0xd000, fpu_m68k, STR("movem"), FMT_0RM_I8, POST_INCR, FP_REGSET>
    , mmu_gen_x <0xf800, fpu_m68k, STR("movem"), FMT_D4_0RM, DATA_REG, CONTROL_ALTER>
    , mmu_gen_x <0xe800, fpu_m68k, STR("movem"), FMT_D4_0RM, DATA_REG, PRE_DECR>
    , mmu_gen_x <0xd800, fpu_m68k, STR("movem"), FMT_0RM_D4, CONTROL, DATA_REG>
    , mmu_gen_x <0xd800, fpu_m68k, STR("movem"), FMT_0RM_D4, POST_INCR, DATA_REG>

// fmovem: fp control registers
    , mmu_gen_l <0xa000, fpu_m68k, STR("movem"), FMT_I13_0RM, FC_REGSET, MEM_ALTER>
    , mmu_gen_l <0x8000, fpu_m68k, STR("movem"), FMT_0RM_I13, MEM, FC_REGSET>

// fmovem: fp control registers: single register formats
    , mmu_gen_l <0xa000, fpu_m68k, STR("movem"), FMT_I13_0RM, FCTRL_REG, DATA_REG>
    , mmu_gen_l <0x8000, fpu_m68k, STR("movem"), FMT_0RM_I13, DATA_REG, FCTRL_REG>
    , mmu_gen_l <0xa000, fpu_m68k, STR("movem"), FMT_I13_0RM, FPIAR, ADDR_REG>
    , mmu_gen_l <0x8000, fpu_m68k, STR("movem"), FMT_0RM_I13, ADDR_REG, FPIAR>
    >;

// store floating point condition code names & values in a type
template <int N, typename NAME>
struct mmu_cc_trait
{
    using type = mmu_cc_trait;
    using code = std::integral_constant<int, N>;
    using name = NAME;
};

// enumerate the floating point condition codes
using mmu_cc_all = list<
      mmu_cc_trait< 0, STR("f")>
    , mmu_cc_trait< 1, STR("eq")>
    , mmu_cc_trait< 2, STR("ogt")>
    , mmu_cc_trait< 3, STR("oge")>
    , mmu_cc_trait< 4, STR("olt")>
    , mmu_cc_trait< 5, STR("ole")>
    , mmu_cc_trait< 6, STR("ogl")>
    , mmu_cc_trait< 7, STR("or")>
    , mmu_cc_trait< 8, STR("un")>
    , mmu_cc_trait< 9, STR("ueq")>
    , mmu_cc_trait<10, STR("ugt")>
    , mmu_cc_trait<11, STR("uge")>
    , mmu_cc_trait<12, STR("ult")>
    , mmu_cc_trait<13, STR("ule")>
    , mmu_cc_trait<14, STR("ne")>
    , mmu_cc_trait<15, STR("t")>
    , mmu_cc_trait<16, STR("sf")>
    , mmu_cc_trait<17, STR("seq")>
    , mmu_cc_trait<18, STR("gt")>
    , mmu_cc_trait<19, STR("ge")>
    , mmu_cc_trait<20, STR("lt")>
    , mmu_cc_trait<21, STR("le")>
    , mmu_cc_trait<22, STR("gl")>
    , mmu_cc_trait<23, STR("gle")>
    , mmu_cc_trait<24, STR("ngle")>
    , mmu_cc_trait<25, STR("ngl")>
    , mmu_cc_trait<26, STR("nle")>
    , mmu_cc_trait<27, STR("nlt")>
    , mmu_cc_trait<28, STR("nge")>
    , mmu_cc_trait<29, STR("ngt")>
    , mmu_cc_trait<30, STR("sne")>
    , mmu_cc_trait<31, STR("st")>
    >;

namespace detail {
    // interface `meta::callable` to `mmu_gen_one`
    struct gen_fpcc_insn
    {
        // shuffle args & extract `OpCode` value
        template <typename OpCode, typename NAME, typename Sz, typename...Args>
        using invoke = mmu_gen_one<Sz, OpCode::value, fpu_m68k, NAME, Args...>;
    };

    // a `meta::callable` to create list of `mmu_gen_one` args
    template <uint32_t, typename...> struct apply_fpcc;

    template <uint32_t OpCode, typename NAME, typename...Args>
    struct apply_fpcc<OpCode, NAME, Args...>
    {
        template <typename T>
        using invoke = list<
                          int_<OpCode + T::code::value>
                        , string::str_cat<NAME, typename T::name>
                        , Args...
                        >;
    };
}

template <uint32_t OpCode, typename...Args>
using fpcc = join<
                transform<
                    transform<mmu_cc_all
                            , detail::apply_fpcc<OpCode, Args...>>
                  , uncurry<detail::gen_fpcc_insn>
              >>;

using mmu_cc_ops = list<list<>
            , fpcc<0x7c << 16, STR("trap"), sz_void>
            , fpcc<0x7a << 16, STR("trap"), sz_w, void, IMMED>
            , fpcc<0x7b << 16, STR("trap"), sz_l, void, IMMED>
            , fpcc<0x40 << 16, STR("s"), sz_bv, FMT_0RM, DATA_ALTER>
            , fpcc<0x48 << 16 | mmu_insn_dbcc
                    , STR("db"), sz_void, FMT_0D, DATA_REG, DIRECT>
            , fpcc<0x80 | mmu_insn_w | mmu_insn_bcc
                    , STR("b"), sz_void, void, DIRECT>
            , fpcc<0x80 | mmu_insn_w | mmu_insn_bcc
                    , STR("j"), sz_void, void, DIRECT>
      >;
#undef STR

using mmu_ops_v = list<mmu_gen_ops, mmu_cc_ops>;
}

namespace kas::m68k::opc
{
    template <> struct m68k_insn_v<OP_M68K_68881> : fp::mmu_ops_v {};
}

#endif
