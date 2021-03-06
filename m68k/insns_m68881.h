#ifndef KAS_M68K_M68881_DEFNS_H
#define KAS_M68K_M68881_DEFNS_H

// Declare floating point processor instrutions
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

#include "m68k_insn_common.h"

namespace kas::m68k::opc::fp
{
using namespace meta;
using namespace hw;

#define STR KAS_STRING

// all coprocessor instructions are F-line instructions
// 040 & ColdFire support requires math coprocessor ID be set to `1`
static constexpr uint32_t fp_cpid   = M68K_CPID_FPU;
static constexpr uint32_t fp_defn_w = 0xf000 + (fp_cpid << 9);
static constexpr uint32_t fp_defn_l = fp_defn_w << 16;
static constexpr uint32_t fp_defn_reg_mem   = 1 << 14;

// Since coprocessor instructions use `F-line` opcodes & fixed coprocessor id
// bits 31-25 are immutable. These 7 bits can be used as flags
// to op-code generation metafunctions, masked away before combined
// with the fixed lsbs.

static constexpr uint32_t fp_defn_use_short = 1 << 31;
static constexpr uint32_t fp_defn_rounding  = 1 << 30;

// mask out 7 bits of flags
static constexpr uint32_t fp_defn_mask      = ((1<<7)-1) << (9 + 16);

// create a cousin of `tgt_opc::OP<>` to handle opcode value manipulation
template <std::size_t OPCODE
        , typename    TST  = hw::fpu_base
        , typename    INFO = INFO_SIZE_FLT
        , std::size_t MASK = 0>
using FOP = OP<!(OPCODE & fp_defn_use_short)
                   ? (fp_defn_l | (OPCODE &~ fp_defn_mask))
                   : (fp_defn_w | (OPCODE &  0xffff))
               , TST
               , INFO
               , MASK
               >;

// create a cousin to `m68k::opc::insn` to
//  1) prepend `f` to name
//  2) mark `SZ` as being floating point
template <typename SZ_LIST, typename NAME, typename FOP, typename...Args>
using fp_defn = defn<meta::int_<SZ_LIST::value | SFX_CPID_FPU>
                           , string::str_cat<STR("f"), NAME>
                           , FOP
                           , Args...>;

// These metafunctions generate the single and dual argument fp instructions
// are split into `interface` and `helper` routines.
// This separation is required to support generation of "040 rounding" instructions
namespace detail {
    template <int Op, typename TST, typename NAME, typename...Args>
    using fpgen_op_all = fp_defn <sz_all
                                , NAME
                                , FOP<Op | fp_defn_reg_mem, TST>
                                , Args...>;

    template <int Op, typename TST, typename NAME, typename...Args>
    using fpgen_op_x   = list<list<>
                , fp_defn <sz_x, NAME, FOP<Op, TST>, Args...>
                , fp_defn <sz_d, NAME, FOP<Op, TST>, Args...>
                >;

    template <int N, typename TST, typename NAME>
    using fp_two = list<list<>
        , fpgen_op_all <(N &~ 0x80), TST, NAME, FMT_0RM_23, DATA, FP_REG>
        , fpgen_op_x   <(N &~ 0x80), TST, NAME, FMT_26_23, FP_REG, FP_REG>
        >;

    template <int N, typename TST, typename NAME>
    using fp_one = list<list<>
        , fp_two <(N &~ 0x80), TST, NAME>
        , fpgen_op_x<(N &~ 0x80), TST, NAME, FMT_DUPL_26_23, FP_REG>
        >;
}


template <int N, typename TST, typename NAME>
using fp_two = list<list<>
        , detail::fp_two<(N &~ 0x80), TST, NAME>
        , if_c<!(N & 0x80), list<>,
                // add in rounding formats
                list<list<>
                    , detail::fp_two<((N &~ 0x80) | 0x40 | fp_defn_rounding)
                                    , TST, string::str_cat<STR("s"), NAME>>
                    , detail::fp_two<((N &~ 0x80) | 0x44 | fp_defn_rounding)
                                    , TST, string::str_cat<STR("d"), NAME>>
                >
            >
        >;

template <int N, typename TST, typename NAME>
using fp_one = list<list<>
        , detail::fp_one<(N &~ 0x80), TST, NAME>
        , if_c<!(N & 0x80), list<>,
                // add in rounding formats
                list<list<>
                    , detail::fp_one<((N &~ 0x80) | 0x40 | fp_defn_rounding)
                                    , TST, string::str_cat<STR("s"), NAME>>
                    , detail::fp_one<((N &~ 0x80) | 0x44 | fp_defn_rounding)
                                    , TST, string::str_cat<STR("d"), NAME>>
                >
            >
        >;

// ftst doesn't support rounding -- so just implement
template <int N, typename TST, typename NAME>
using fp_tst = list<list<>
        , detail::fpgen_op_all<(N &~ 0x80), TST, NAME, FMT_0RM, DATA>
        , detail::fpgen_op_x  <(N &~ 0x80), TST, NAME, FMT_26, FP_REG>
        >;

// for `monadic` & `dyadic` instructions (*only* fp_one & fp_two):
// op-mode is 7 lsbs of second word.
// op-mode 0x80  bit indicates rounding modes must also be generated
// NB: all cpu implementations but no coprocessors support rounding...

// NB: each entry generates between 8 and 27 distinct instructions

using fp_gen_ops = list<list<>
// opcodes using "regular" opcode formats
    , fp_two<0x80, fpu_basic, STR("move")>
    , fp_one<0x01, fpu_intrz, STR("int")>
    , fp_one<0x02, fpu_trig,  STR("sinh")>
    , fp_one<0x03, fpu_intrz, STR("intrz")>
    , fp_one<0x84, fpu_basic, STR("sqrt")>
    , fp_one<0x06, fpu_trig,  STR("lognp1")>
    , fp_one<0x08, fpu_trig,  STR("etoxm1")>
    , fp_one<0x09, fpu_trig,  STR("tanh")>
    , fp_one<0x0a, fpu_trig,  STR("atan")>
    , fp_one<0x0c, fpu_trig,  STR("asin")>
    , fp_one<0x0d, fpu_trig,  STR("atanh")>
    , fp_one<0x0e, fpu_trig,  STR("sin")>
    , fp_one<0x0f, fpu_trig,  STR("tan")>
    , fp_one<0x10, fpu_trig,  STR("etox")>
    , fp_one<0x11, fpu_trig,  STR("twotox")>
    , fp_one<0x12, fpu_trig,  STR("tentox")>
    , fp_one<0x14, fpu_trig,  STR("logn")>
    , fp_one<0x15, fpu_trig,  STR("log10")>
    , fp_one<0x16, fpu_trig,  STR("log2")>
    , fp_one<0x98, fpu_basic, STR("abs")>
    , fp_one<0x19, fpu_trig,  STR("cosh")>
    , fp_one<0x9a, fpu_basic, STR("neg")>
    , fp_one<0x1c, fpu_trig,  STR("acos")>
    , fp_one<0x1d, fpu_trig,  STR("cos")>
    , fp_one<0x1e, fpu_trig,  STR("getexp")>
    , fp_one<0x1f, fpu_trig,  STR("getman")>
    , fp_two<0xa0, fpu_basic, STR("div")>
    , fp_two<0x21, fpu_m68k,  STR("mod")>
    , fp_two<0xa2, fpu_basic, STR("add")>
    , fp_two<0xa3, fpu_basic, STR("mul")>
    , fp_two<0x24, fpu_m68k,  STR("sgldiv")>
    , fp_two<0x25, fpu_m68k,  STR("rem")>
    , fp_two<0x26, fpu_trig,  STR("scale")>
    , fp_two<0x27, fpu_m68k,  STR("sglmul")>
    , fp_two<0xa8, fpu_basic, STR("sub")>
    , fp_two<0x38, fpu_basic, STR("cmp")>
    , fp_tst<0x3a, fpu_m68k,  STR("tst")>

// sincos is special (three arguments)
, fp_defn <sz_all, STR("sincos"), FOP<0x30 | fp_defn_reg_mem, fpu_trig>
                        , FMT_0RM_16_23, DATA, FP_REG, FP_REG>
, fp_defn <sz_x, STR("sincos"), FOP<0x30, fpu_trig>
                        , FMT_26_16_23, FP_REG, FP_REG, FP_REG>

// fmove special instructions:: kfactor & constant ROM
, fp_defn <sz_x , STR("movecr"), FOP<0x5c00, fpu_trig>
                        , FMT_I7_23, Q_7BITS, FP_REG>
    // XXX don't know assembler format of "fortran" insns
    // , fpgen_p  <0x00, STR("move"), FP_REG, DATA, KF_DYNAMIC>
    // , fpgen_p  <0x00, STR("move"), FP_REG, DATA, KF_STATIC>

// fmove: fp control registers
, fp_defn <sz_l, STR("move"), FOP<0x8000, fpu_basic>
                        , FMT_0RM_26, DATA, FCTRL_REG>
, fp_defn <sz_l, STR("move"), FOP<0xa000, fpu_basic>
                        , FMT_26_0RM, FCTRL_REG, DATA_ALTER>

// fmove: FPIAR can also use address registers
, fp_defn <sz_l, STR("move"), FOP<0x8000, fpu_basic>
                        , FMT_0RM_26, ADDR_REG, FPIAR>
, fp_defn <sz_l, STR("move"), FOP<0xa000, fpu_basic>
                        , FMT_26_0RM, FPIAR, ADDR_REG>

// fsave/frestore
, fp_defn <sz_v, STR("save"), FOP<0x100 | fp_defn_use_short, fpu_basic>
                        , FMT_0RM, CONTROL_ALTER>
, fp_defn <sz_v, STR("save"), FOP<0x100 | fp_defn_use_short, fpu_m68k>
                        , FMT_0RM, PRE_DECR>
, fp_defn <sz_v, STR("restore"), FOP<0x140 | fp_defn_use_short, fpu_m68k>
                        , FMT_0RM, CONTROL>
, fp_defn <sz_v, STR("restore"), FOP<0x140 | fp_defn_use_short, fpu_m68k>
                        , FMT_0RM, POST_INCR>

// fmovem: fp registers: static list & dynamic (via DATA REGISTER) versions
, fp_defn <sz_x, STR("movem"), FOP<0xf000, fpu_m68k>
                        , FMT_I8_0RM, FP_REGSET_REV, CONTROL_ALTER>
, fp_defn <sz_x, STR("movem"), FOP<0xe000, fpu_m68k>
                        , FMT_I8_0RM, FP_REGSET, PRE_DECR>
, fp_defn <sz_x, STR("movem"), FOP<0xd000, fpu_m68k>
                        , FMT_0RM_I8, CONTROL, FP_REGSET_REV>
, fp_defn <sz_x, STR("movem"), FOP<0xd000, fpu_m68k>
                        , FMT_0RM_I8, POST_INCR, FP_REGSET_REV>
, fp_defn <sz_x, STR("movem"), FOP<0xf800, fpu_m68k>
                        , FMT_20_0RM, DATA_REG, CONTROL_ALTER>
, fp_defn <sz_x, STR("movem"), FOP<0xe800, fpu_m68k>
                        , FMT_20_0RM, DATA_REG, PRE_DECR>
, fp_defn <sz_x, STR("movem"), FOP<0xd800, fpu_m68k>
                        , FMT_0RM_20, CONTROL, DATA_REG>
, fp_defn <sz_x, STR("movem"), FOP<0xd800, fpu_m68k>
                        , FMT_0RM_20, POST_INCR, DATA_REG>

// fmovem: fp control registers
, fp_defn <sz_l, STR("movem"), FOP<0xa000, fpu_m68k>
                        , FMT_26_0RM, FC_REGSET, MEM_ALTER>
, fp_defn <sz_l, STR("movem"), FOP<0x8000, fpu_m68k>
                        , FMT_0RM_26, MEM, FC_REGSET>

// fmovem: fp control registers: single register formats
, fp_defn <sz_l, STR("movem"), FOP<0xa000, fpu_m68k>
                        , FMT_26_0RM, FCTRL_REG, DATA_REG>
, fp_defn <sz_l, STR("movem"), FOP<0x8000, fpu_m68k>
                        , FMT_0RM_26, DATA_REG, FCTRL_REG>
, fp_defn <sz_l, STR("movem"), FOP<0xa000, fpu_m68k>
                        , FMT_26_0RM, FPIAR, ADDR_REG>
, fp_defn <sz_l, STR("movem"), FOP<0x8000, fpu_m68k>
                        , FMT_0RM_26, ADDR_REG, FPIAR>
>;

using fp_cc_ops = list<list<>
, fp_defn <sz_v , STR("trap"), FOP<0x7c'0000, fpu_basic, INFO_CCODE_FP1>>
, fp_defn <sz_w , STR("trap"), FOP<0x7a'0000, fpu_basic, INFO_CCODE_FP1>
                                    , FMT_X, IMMED>
, fp_defn <sz_l , STR("trap"), FOP<0x7b'0000, fpu_basic, INFO_CCODE_FP1>
                                    , FMT_X, IMMED>
, fp_defn <sz_bv, STR("s")   , FOP<0x40'0000, fpu_basic, INFO_CCODE_FP1> 
                                    , FMT_0RM, DATA_ALTER>
, fp_defn <sz_v , STR("db")  , FOP<0x48'0000, fpu_basic, INFO_CCODE_FP1>
                                    , FMT_DBCC, DATA_REG, DIRECT>
// XXX displacement 16/32 is bit-6. not std branch
, fp_defn <sz_v , STR("b"), FOP<0x80 | fp_defn_use_short, fpu_basic, INFO_CCODE_FP0>
                                    , FMT_CP_BRANCH, BRANCH_WL_DEL>
, fp_defn <sz_v , STR("j"), FOP<0x80 | fp_defn_use_short, fpu_basic, INFO_CCODE_FP0>
                                    , FMT_CP_BRANCH, BRANCH_WL_DEL>

      >;
#undef STR

using fp_ops_v = list<fp_gen_ops, fp_cc_ops>;
}

namespace kas::m68k::opc
{ 
    template <> struct m68k_insn_defn_list<OP_M68K_FPU> : fp::fp_ops_v {};
}

#endif
