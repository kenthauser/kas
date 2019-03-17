#ifndef KAS_M68K_M68K_FORMATS_DEFN_H
#define KAS_M68K_M68K_FORMATS_DEFN_H

// 1. Remove `index` infrastructure
// 2. Split into virtual functions, "workers" and combiners
// 3. Add in `opc&` stuff

#include "m68k_formats_impl.h"
#include "m68k_formats_opc.h"

namespace kas::m68k::opc
{
// declare mixin classes to override virtual functions
// override insert/extract pairs together...

// `reg_mode` inserts 3-bit `cpu_reg` & 3-bit `cpu_mode()`
using gen_reg_mode = reg_mode<0>;
using mov_reg_mode = reg_mode<9, 0, -3>;
using ext_reg_mode = reg_mode<12, 1>;
//using fmt_bitfield = fmt_bitfield;

#if 0
// `fmt_reg` is limited to 3-bit `cpu_reg` register values
// needs "register mode" for disassembler
template <int N, int W=0> using fmt_data    = fmt_reg<N, MODE_DATA_REG   , W>;
template <int N, int W=0> using fmt_addr    = fmt_reg<N, MODE_ADDR_REG   , W>;
template <int N, int W=0> using fmt_indir   = fmt_reg<N, MODE_ADDR_INDIR , W>;
template <int N, int W=0> using fmt_postinc = fmt_reg<N, MODE_POST_INCR  , W>;
template <int N, int W=0> using fmt_predec  = fmt_reg<N, MODE_PRE_DECR   , W>;
template <int N, int W=0> using fmt_flt     = fmt_reg<N, 0x100 | RC_FLOAT, W>;
template <int N, int W=0> using fmt_fctrl   = fmt_reg<N, 0x100 | RC_FCTRL, W>;
#else
template <int SHIFT, int WORD = 0>
using fmt_reg = fmt_generic<SHIFT, 3, WORD>;

#endif

// associate format function with source/dest argument
// 3-bit reg# stored in four places
using arg1_0   = fmt_arg<1, fmt_reg<0>>;
using arg1_9   = fmt_arg<1, fmt_reg<9>>;
using arg1_1w0 = fmt_arg<1, fmt_reg<0, 1>>;
using arg1_1w4 = fmt_arg<1, fmt_reg<4, 1>>;

using arg2_0   = fmt_arg<2, fmt_reg<0>>;
using arg2_9   = fmt_arg<2, fmt_reg<9>>;
using arg2_1w0 = fmt_arg<2, fmt_reg<0, 1>>;
using arg2_1w4 = fmt_arg<2, fmt_reg<4, 1>>;

using arg3_1w0  = fmt_arg<3, fmt_reg<0, 1>>;

// 6-bit reg/mode stored two places
using arg1_0rm  = fmt_arg<1, gen_reg_mode>;
using arg1_1w12 = fmt_arg<1, ext_reg_mode>;

using arg2_0rm  = fmt_arg<2, gen_reg_mode>;
using arg2_1w12 = fmt_arg<2, ext_reg_mode>;

using arg3_0rm  = fmt_arg<3, gen_reg_mode>;
using arg3_1w12 = fmt_arg<3, ext_reg_mode>;
using arg3_12rm = fmt_arg<3, reg_mode<12>>;     // save general reg# for list

// special 6-bit reg/mode for `move` dest
using arg2_6rm  = fmt_arg<2, mov_reg_mode>;

// quick immediate args: always LSBs, but varying widths
template <int N, int W = 0, bool REVERSE = false>
using arg1_i   = fmt_arg<1, fmt_generic<0, N, W>>;

template <int N, int W = 0, bool REVRSE = false>
using arg2_i   = fmt_arg<2, fmt_generic<0, N, W>>;

//using arg2_12p  = fmt_arg<2, fmt_postinc<12, 1>>;
//using arg2_22d  = fmt_arg<2, fmt_data<6, 1>>;
//busing arg2_d4   = fmt_arg<2, fmt_data<4, 1>>;

//using arg2_bf   = fmt_arg<2, fmt_bitfield>;

//using arg2_mult_pair = fmt_arg<2, fmt_mult_pair>;

//using arg3_bf   = fmt_arg<3, fmt_bitfield>;


#if 0

// coldfire mac/emac formats
using arg4_0rm  = fmt_arg<4, gen_reg_mode>;

using arg1_0an   = fmt_arg<1, fmt_accn<0>>;
using arg1_9an   = fmt_arg<1, fmt_accn<9>>;
using arg2_9an   = fmt_arg<2, fmt_accn<9>>;
using arg3_7an  = fmt_arg<3, fmt_accn<7>>; 
using arg4_7an  = fmt_arg<4, fmt_accn<7>>; 
using arg5_7an  = fmt_arg<5, fmt_accn<7>>; 
using arg6_7an  = fmt_arg<6, fmt_accn<7>>; 
using arg4_an2  = fmt_arg<4, fmt_accn<2, 1>>; 
using arg5_an2  = fmt_arg<5, fmt_accn<2, 1>>; 

// LSB of accumulator inverted. eMAC MACL
using arg5_i7an = fmt_arg<5, fmt_accn<7, 0, 1>>; 
using arg6_i7an = fmt_arg<6, fmt_accn<7, 0, 1>>; 

// support subwords in MAC & eMAC opcodes
using arg1_0ul   = fmt_arg<1, fmt_regul<0,  0, 1>>;
using arg1_ul0   = fmt_arg<1, fmt_regul<0,  1, 1>>;
using arg2_9ul   = fmt_arg<2, fmt_regul<9,  0, 2>>;
using arg2_ul12  = fmt_arg<2, fmt_regul<12, 1, 2>>;

using arg4_9ul  = fmt_arg<4, fmt_regul<9>>;
using arg5_9ul  = fmt_arg<5, fmt_regul<9>>;

// immediate args are less regular
// NB: anything with 5 template arguments is not regular
using arg1_9q   = fmt_arg<1, fmt_immed<9, 3, 0, false, q_math>>;
using arg1_9qm  = fmt_arg<1, fmt_immed<9, 3, 0, false, q_mov3q>>;

template <int N>
using arg1_0i = fmt_arg<1, fmt_immed<0, N>>;

// DIR is because "movem" bits are reversed in some cases. 
// Need flag for these cases.
template <int N, unsigned WORD, bool DIR = false>
using arg1_i  = fmt_arg<1, fmt_immed<0, N, WORD, DIR>>;

template <int N, unsigned WORD, bool DIR = false>
using arg2_i  = fmt_arg<2, fmt_immed<0, N, WORD, DIR>>;

// Floating point modes are different...
// FP "source specifier" field
using arg1_7fp  = fmt_arg<1, fmt_flt<7, 1>>;
using arg1_10fp = fmt_arg<1, fmt_flt<10, 1>>;
using arg1_10fc = fmt_arg<1, fmt_fctrl<10, 1>>;

// FP "duplicate single arg in src & dst fields"
using arg1_arg2_fp = fmt_arg<1, fmt_flt_duplicate<1, 7, 10>>;

using arg2_7fp    = fmt_arg<2, fmt_flt<7, 1>>;
using arg2_0fp    = fmt_arg<2, fmt_flt<0, 1>>;
using arg3_7fp   = fmt_arg<3, fmt_flt<7, 1>>;

using arg2_10fc   = fmt_arg<2, fmt_fctrl<10, 1>>;

// specials which "extract" fixed register, but insert nothing
using arg1_sr     = fmt_arg<1, fmt_reg_direct<REG_CPU_SR>>;
using arg2_sr     = fmt_arg<2, fmt_reg_direct<REG_CPU_SR>>;
using arg1_ccr    = fmt_arg<1, fmt_reg_direct<REG_CPU_CCR>>;
using arg2_ccr    = fmt_arg<2, fmt_reg_direct<REG_CPU_CCR>>;
using arg1_usp    = fmt_arg<1, fmt_reg_direct<REG_CPU_USP>>;
using arg2_usp    = fmt_arg<2, fmt_reg_direct<REG_CPU_USP>>;

// special for clr spelled move #0
using arg1_zero   = fmt_arg<1, fmt_zero>;
#endif

///////////////////////////////////////////////////////////////////////////


// used by OPC_LIST instructions
struct FMT_LIST     : fmt_list, arg1_0rm, arg2_6rm, arg3_12rm {};


// branch formats have implied argument format
using FMT_BRANCH     = fmt_branch;
using FMT_CP_BRANCH  = fmt_cp_branch;

// dbcc have single argument
struct FMT_DBCC     : fmt_dbcc,    arg1_0 {};
struct FMT_CP_DBCC  : fmt_cp_dbcc, arg1_0 {};

// name template: FMT_ <First Arg (src)> _ <Second Arg (dst)>
struct FMT_X        : fmt_gen {};

struct FMT_0        : fmt_gen, arg1_0 {};
struct FMT_0RM      : fmt_gen, arg1_0rm {};

struct FMT_0_9      : fmt_gen, arg1_0, arg2_9 {};
struct FMT_0RM_9    : fmt_gen, arg1_0rm, arg2_9 {};

struct FMT_9_0      : fmt_gen, arg1_9, arg2_0 {};
struct FMT_9_0RM    : fmt_gen, arg1_9, arg2_0rm {};

struct FMT_X_0      : fmt_gen, arg2_0 {};
struct FMT_X_0RM    : fmt_gen, arg2_0rm {};

// special for move general/general
struct FMT_0RM_6RM  : fmt_gen, arg1_0rm, arg2_6rm {};

// move to/from m68k control reg (reg & immed)
struct FMT_0RM_12RM : fmt_gen, arg1_0rm, arg2_1w12 {};
struct FMT_12RM_0RM : fmt_gen, arg2_0rm, arg1_1w12 {};
struct FMT_I12_12RM : fmt_gen, arg1_i<12, 1>, arg2_1w12 {};
struct FMT_12RM_I12 : fmt_gen, arg2_i<12, 1>, arg1_1w12 {};

// Quick Immed 1-word formats
struct FMT_3I       : fmt_gen, arg1_i<3> {};    // aka FMT_0 (for trap)
struct FMT_4I       : fmt_gen, arg1_i<4> {};
struct FMT_8I_9     : fmt_gen, arg1_i<8>, arg2_9 {};

// Quick Immed 2-word formats
struct FMT_0RM_I16  : fmt_gen, arg1_0rm, arg2_i<16, 1> {};
struct FMT_I16_0RM  : fmt_gen, arg1_i<16, 1>, arg2_0rm {};
struct FMT_I8_0RM   : fmt_gen, arg1_i<8, 1>, arg2_0rm {};

// special for movem to memory PRE_DECR
struct FMT_I16R_0RM : fmt_gen, arg1_i<16, 1, true>, arg2_0rm {};

// special for STOP
struct FMT_I16      : fmt_gen, arg1_i<16, 1> {};

#if 0
// support movec
struct FMT_I12_12RM  : fmt_gen, arg1_i<12, 1>, arg2_12rm {};
struct FMT_12RM_I12  : fmt_gen, arg1_12rm, arg2_i<12, 1> {};

// 32/64 bit multiply divide
struct FMT_12RM_0RM : fmt_gen, arg1_12rm, arg2_0rm {};
struct FMT_0RM_12RM : fmt_gen, arg1_0rm, arg2_12rm {};
struct FMT_0RM_PAIR : fmt_gen, arg1_0rm, arg2_mult_pair {};

// bitfield instructions
struct FMT_0RM_BF      : fmt_gen, arg1_0rm, arg2_bf {};
struct FMT_0RM_BF_12RM : fmt_gen, arg1_0rm, arg2_bf, arg3_12rm {};
struct FMT_12RM_0RM_BF : fmt_gen, arg1_12rm, arg2_0rm, arg3_bf {};

// move16
struct FMT_0P_12P   : fmt_gen, arg1_0p, arg2_12p {};
struct FMT_0P       : fmt_gen, arg1_0p {};
struct FMT_0S       : fmt_gen, arg1_0s {};
struct FMT_X_0P     : fmt_gen, arg2_0p {};
struct FMT_X_0S     : fmt_gen, arg2_0s {};

// cas & cas2 (CAS2 is unique)
struct FMT_16D_22D_0RM : fmt_gen, arg1_16d, arg2_22d, arg3_0rm {};
struct FMT_CAS2     : fmt_cas2
                    , fmt_arg<1, fmt_tbl_pair<0>>
                    , fmt_arg<2, fmt_tbl_pair<6>>
                    , fmt_arg<3, fmt_cas2_mem_pair>
                    {};

// CPU32 specials
struct FMT_TBLPAIR_12RM : fmt_gen, arg1_tbl_pair, arg2_0rm {};

//
// Floating point formats
//

struct FMT_0RM_7FP  : fmt_gen, arg1_0rm, arg2_7fp {};
struct FMT_10FP_7FP : fmt_gen, arg1_10fp, arg2_7fp {};
struct FMT_10FP     : fmt_gen, arg1_10fp {};
struct FMT_DUPL_FP  : fmt_gen, arg1_arg2_fp {};

// Specials for FCTRL
struct FMT_0RM_10FC : fmt_gen, arg1_0rm, arg2_10fc {};
struct FMT_10FC_0RM : fmt_gen, arg1_10fc, arg2_0rm {};

// Specials for `fsincos`
struct FMT_0RM_0FP_7FP  : fmt_gen, arg1_0rm,  arg2_0fp, arg3_7fp {};
struct FMT_10FP_0FP_7FP : fmt_gen, arg1_10fp, arg2_0fp, arg3_7fp {};

// Special for `fmovecr`: shift=0, bits=7, word=1
struct FMT_CR0_7FP  : fmt_gen, arg1_i<7, 1>, arg2_7fp {};

// fmovem.x: 5 Formats: From Mem, To Mem, PreDecr, Static vs Dynamic
// only 4 are new defns.
struct FMT_0RM_I8    : fmt_gen, arg1_0rm, arg2_i<8, 1> {};
struct FMT_I8R_0RM   : fmt_gen, arg1_i<8, 1, true>, arg2_0rm {};
struct FMT_0RM_D4    : fmt_gen, arg1_0rm, arg2_d4 {};
struct FMT_D4_0RM    : fmt_gen, arg1_d4, arg2_0rm {};

// fmovem.l: 2 Formats: From Mem, To Mem
struct FMT_0RM_I13   : fmt_gen, arg1_0rm, arg2_i<13, 1> {};
struct FMT_I13_0RM   : fmt_gen, arg1_i<13, 1>, arg2_0rm {};


// coldfire formats
// formats for emac multiple accumulators (ACC_N)
struct FMT_9AN_0RM  : fmt_gen, arg1_9an, arg2_0rm {};
struct FMT_0RM_9AN  : fmt_gen, arg1_0rm, arg2_9an {};
struct FMT_0AN_9AN  : fmt_gen, arg1_0an, arg2_9an {};

// mac MAC
struct FMT_0UL_9UL  : fmt_gen, arg1_0ul, arg2_9ul {};

// mac MACL
struct FMT_UL0_UL12_0RM_9UL   : fmt_gen, arg1_ul0, arg2_ul12, arg3_0rm, arg4_9ul {};
struct FMT_UL0_UL12_X_0RM_9UL : fmt_gen, arg1_ul0, arg2_ul12, arg4_0rm, arg5_9ul {};

// emac MAC
struct FMT_0UL_9UL_7AN   : fmt_gen, arg1_0ul, arg2_9ul, arg3_7an {};
struct FMT_0UL_9UL_X_7AN : fmt_gen, arg1_0ul, arg2_9ul, arg4_7an {};

// emac MACL (note Accumulator LSB is inverted)
struct FMT_UL0_UL12_0RM_9UL_I7AN   : fmt_gen, arg1_ul0, arg2_ul12, arg3_0rm, arg4_9ul, arg5_i7an {};
struct FMT_UL0_UL12_X_0RM_9UL_I7AN : fmt_gen, arg1_ul0, arg2_ul12, arg4_0rm, arg5_9ul, arg6_i7an {};

// emac_v2 Dual ACC insns
struct FMT_0UL_9UL_7AN_AN2   : fmt_gen, arg1_ul0, arg2_9ul, arg3_7an, arg4_an2 {};
struct FMT_0UL_9UL_X_7AN_AN2 : fmt_gen, arg1_ul0, arg2_9ul, arg4_7an, arg5_an2 {};
#endif
}
#endif


