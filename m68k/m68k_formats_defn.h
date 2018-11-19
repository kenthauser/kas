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
using fmt_bitfield = fmt_bitfield;

// `fmt_reg` is limited to 3-bit `cpu_reg` register values
// needs "register mode" for disassembler
template <int N, int W=0> using fmt_data    = fmt_reg<N, MODE_DATA_REG   , W>;
template <int N, int W=0> using fmt_addr    = fmt_reg<N, MODE_ADDR_REG   , W>;
template <int N, int W=0> using fmt_indir   = fmt_reg<N, MODE_ADDR_INDIR , W>;
template <int N, int W=0> using fmt_postinc = fmt_reg<N, MODE_POST_INCR  , W>;
template <int N, int W=0> using fmt_predec  = fmt_reg<N, MODE_PRE_DECR   , W>;
template <int N, int W=0> using fmt_flt     = fmt_reg<N, 0x100 | RC_FLOAT, W>;
template <int N, int W=0> using fmt_fctrl   = fmt_reg<N, 0x100 | RC_FCTRL, W>;


// associate format function with source/dest argument
using src_0d   = fmt_arg<1, fmt_data<0>>;
using src_0a   = fmt_arg<1, fmt_addr<0>>;
using src_0s   = fmt_arg<1, fmt_indir<0>>;
using src_0p   = fmt_arg<1, fmt_postinc<0>>;
using src_0m   = fmt_arg<1, fmt_predec<0>>;

using src_9d   = fmt_arg<1, fmt_data<9>>;
using src_9a   = fmt_arg<1, fmt_addr<9>>;
using src_16d  = fmt_arg<1, fmt_data<0, 1>>;
using src_d4   = fmt_arg<1, fmt_data<4, 1>>;

using src_0rm  = fmt_arg<1, gen_reg_mode>;
using src_12rm = fmt_arg<1, ext_reg_mode>;

using src_tbl_pair = fmt_arg<1, fmt_tbl_pair<>>;

using dst_0d   = fmt_arg<2, fmt_data<0>>;
using dst_0a   = fmt_arg<2, fmt_addr<0>>;
using dst_0s   = fmt_arg<2, fmt_indir<0>>;
using dst_0p   = fmt_arg<2, fmt_postinc<0>>;
using dst_0m   = fmt_arg<2, fmt_predec<0>>;

using dst_9d   = fmt_arg<2, fmt_data<9>>;
using dst_9a   = fmt_arg<2, fmt_addr<9>>;
using dst_9p   = fmt_arg<2, fmt_postinc<9>>;
using dst_9m   = fmt_arg<2, fmt_predec<9>>;

using dst_12p  = fmt_arg<2, fmt_postinc<12, 1>>;
using dst_22d  = fmt_arg<2, fmt_data<6, 1>>;
using dst_d4   = fmt_arg<2, fmt_data<4, 1>>;

using dst_0rm  = fmt_arg<2, gen_reg_mode>;
using dst_6rm  = fmt_arg<2, mov_reg_mode>;
using dst_12rm = fmt_arg<2, ext_reg_mode>;
using dst_bf   = fmt_arg<2, fmt_bitfield>;

using dst_mult_pair = fmt_arg<2, fmt_mult_pair>;

using arg3_0rm  = fmt_arg<3, gen_reg_mode>;
using arg3_16d  = fmt_arg<3, fmt_data<0, 1>>;
using arg3_12rm = fmt_arg<3, ext_reg_mode>;
using arg3_bf   = fmt_arg<3, fmt_bitfield>;


// coldfire mac/emac formats
using arg4_0rm  = fmt_arg<4, gen_reg_mode>;

using src_0an   = fmt_arg<1, fmt_accn<0>>;
using src_9an   = fmt_arg<1, fmt_accn<9>>;
using dst_9an   = fmt_arg<2, fmt_accn<9>>;
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
using src_0ul   = fmt_arg<1, fmt_regul<0,  0, 1>>;
using src_ul0   = fmt_arg<1, fmt_regul<0,  1, 1>>;
using dst_9ul   = fmt_arg<2, fmt_regul<9,  0, 2>>;
using dst_ul12  = fmt_arg<2, fmt_regul<12, 1, 2>>;

using arg4_9ul  = fmt_arg<4, fmt_regul<9>>;
using arg5_9ul  = fmt_arg<5, fmt_regul<9>>;

// immediate args are less regular
// NB: anything with 5 template arguments is not regular
using src_9q   = fmt_arg<1, fmt_immed<9, 3, 0, false, q_math>>;
using src_9qm  = fmt_arg<1, fmt_immed<9, 3, 0, false, q_mov3q>>;

template <int N>
using src_0i = fmt_arg<1, fmt_immed<0, N>>;

// DIR is because "movem" bits are reversed in some cases. 
// Need flag for these cases.
template <int N, unsigned WORD, bool DIR = false>
using src_i  = fmt_arg<1, fmt_immed<0, N, WORD, DIR>>;

template <int N, unsigned WORD, bool DIR = false>
using dst_i  = fmt_arg<2, fmt_immed<0, N, WORD, DIR>>;

// Floating point modes are different...
// FP "source specifier" field
using src_7fp  = fmt_arg<1, fmt_flt<7, 1>>;
using src_10fp = fmt_arg<1, fmt_flt<10, 1>>;
using src_10fc = fmt_arg<1, fmt_fctrl<10, 1>>;

// FP "duplicate single arg in src & dst fields"
using src_dst_fp = fmt_arg<1, fmt_flt_duplicate<1, 7, 10>>;

using dst_7fp    = fmt_arg<2, fmt_flt<7, 1>>;
using dst_0fp    = fmt_arg<2, fmt_flt<0, 1>>;
using arg3_7fp   = fmt_arg<3, fmt_flt<7, 1>>;

using dst_10fc   = fmt_arg<2, fmt_fctrl<10, 1>>;

// specials which "extract" fixed register, but insert nothing
using src_sr     = fmt_arg<1, fmt_reg_direct<REG_CPU_SR>>;
using dst_sr     = fmt_arg<2, fmt_reg_direct<REG_CPU_SR>>;
using src_ccr    = fmt_arg<1, fmt_reg_direct<REG_CPU_CCR>>;
using dst_ccr    = fmt_arg<2, fmt_reg_direct<REG_CPU_CCR>>;
using src_usp    = fmt_arg<1, fmt_reg_direct<REG_CPU_USP>>;
using dst_usp    = fmt_arg<2, fmt_reg_direct<REG_CPU_USP>>;

// special for clr spelled move #0
using src_zero   = fmt_arg<1, fmt_zero>;

///////////////////////////////////////////////////////////////////////////


// used by OPC_LIST instructions
struct FMT_LIST     : fmt_list, src_0rm, dst_6rm {};

// define static member function
m68k_opcode_fmt const& m68k_opcode_fmt::get_list_fmt()
{
    static const FMT_LIST fmt;
    return fmt;
}

// branch formats have implied argument format
using FMT_BRANCH     = fmt_branch;
using FMT_CP_BRANCH  = fmt_cp_branch;

// dbcc have single argument
struct FMT_DBCC     : fmt_dbcc,    src_0d {};
struct FMT_CP_DBCC  : fmt_cp_dbcc, src_0d {};

// name template: FMT_ <First Arg (src)> _ <Second Arg (dst)>
struct FMT_0A       : fmt_gen, src_0a {};
struct FMT_0D       : fmt_gen, src_0d {};
struct FMT_0I3      : fmt_gen, src_0i<3> {};
struct FMT_0I4      : fmt_gen, src_0i<4> {};
struct FMT_0I8_9D   : fmt_gen, src_0i<8>, dst_9d {};
struct FMT_0RM      : fmt_gen, src_0rm {};
struct FMT_0RM_9A   : fmt_gen, src_0rm, dst_9a {};
struct FMT_0RM_9D   : fmt_gen, src_0rm, dst_9d {};
struct FMT_9A_0RM   : fmt_gen, src_9a, dst_0rm {};
struct FMT_9D_0RM   : fmt_gen, src_9d, dst_0rm {};
struct FMT_9Q_0RM   : fmt_gen, src_9q, dst_0rm {};
struct FMT_9Q_0D    : fmt_gen, src_9q, dst_0d {};

struct FMT_9QM_0RM  : fmt_gen, src_9qm, dst_0rm {};

struct FMT_MOV_0RM_6RM : fmt_gen, src_0rm, dst_6rm {};

// move USP, etc
struct FMT_USP_0A   : fmt_gen, src_usp, dst_0a {};
struct FMT_0A_USP   : fmt_gen, src_0a, dst_usp {};
struct FMT_SR_0RM   : fmt_gen, src_sr, dst_0rm {};
struct FMT_0RM_SR   : fmt_gen, src_0rm, dst_sr {};
struct FMT_CCR_0RM  : fmt_gen, src_ccr, dst_0rm {};
struct FMT_0RM_CCR  : fmt_gen, src_0rm, dst_ccr {};

struct FMT_X_0RM    : fmt_gen, dst_0rm {};

// special for clr.l spelled as moveq #0
struct FMT_Z_0RM    : fmt_gen, src_zero, dst_0rm {};

// exg
struct FMT_9D_0D    : fmt_gen, src_9d, dst_0d {};
struct FMT_9A_0A    : fmt_gen, src_9a, dst_0a {};
struct FMT_9A_0D    : fmt_gen, src_9a, dst_0d {};
struct FMT_0D_9A    : fmt_gen, src_0d, dst_9a {};

struct FMT_0D_9D    : fmt_gen, src_0d, dst_9d {};
struct FMT_0M_9M    : fmt_gen, src_0m, dst_9m {};
struct FMT_0P_9P    : fmt_gen, src_0p, dst_9p {};

// movep XXX
struct FMT_0A_9D    : fmt_gen, src_0a, dst_9d {};
struct FMT_9D_0A    : fmt_gen, src_9d, dst_0a {};

// two word formats
struct FMT_0RM_I16   : fmt_gen, src_0rm, dst_i<16, 1> {};
struct FMT_I16_0RM   : fmt_gen, src_i<16, 1>, dst_0rm {};
struct FMT_I8_0RM    : fmt_gen, src_i<8, 1>, dst_0rm {};

// special for movem to memory PRE_DECR
struct FMT_I16R_0RM  : fmt_gen, src_i<16, 1, true>, dst_0rm {};

// special for STOP
struct FMT_I16       : fmt_gen, src_i<16, 1> {};

// support movec
struct FMT_I12_12RM  : fmt_gen, src_i<12, 1>, dst_12rm {};
struct FMT_12RM_I12  : fmt_gen, src_12rm, dst_i<12, 1> {};

// 32/64 bit multiply divide
struct FMT_12RM_0RM : fmt_gen, src_12rm, dst_0rm {};
struct FMT_0RM_12RM : fmt_gen, src_0rm, dst_12rm {};
struct FMT_0RM_PAIR : fmt_gen, src_0rm, dst_mult_pair {};

// bitfield instructions
struct FMT_0RM_BF      : fmt_gen, src_0rm, dst_bf {};
struct FMT_0RM_BF_12RM : fmt_gen, src_0rm, dst_bf, arg3_12rm {};
struct FMT_12RM_0RM_BF : fmt_gen, src_12rm, dst_0rm, arg3_bf {};

// move16
struct FMT_0P_12P   : fmt_gen, src_0p, dst_12p {};
struct FMT_0P       : fmt_gen, src_0p {};
struct FMT_0S       : fmt_gen, src_0s {};
struct FMT_X_0P     : fmt_gen, dst_0p {};
struct FMT_X_0S     : fmt_gen, dst_0s {};

// cas & cas2 (CAS2 is unique)
struct FMT_16D_22D_0RM : fmt_gen, src_16d, dst_22d, arg3_0rm {};
struct FMT_CAS2     : fmt_cas2
                    , fmt_arg<1, fmt_tbl_pair<0>>
                    , fmt_arg<2, fmt_tbl_pair<6>>
                    , fmt_arg<3, fmt_cas2_mem_pair>
                    {};

// CPU32 specials
struct FMT_TBLPAIR_12RM : fmt_gen, src_tbl_pair, dst_0rm {};

//
// Floating point formats
//

struct FMT_0RM_7FP  : fmt_gen, src_0rm, dst_7fp {};
struct FMT_10FP_7FP : fmt_gen, src_10fp, dst_7fp {};
struct FMT_10FP     : fmt_gen, src_10fp {};
struct FMT_DUPL_FP  : fmt_gen, src_dst_fp {};

// Specials for FCTRL
struct FMT_0RM_10FC : fmt_gen, src_0rm, dst_10fc {};
struct FMT_10FC_0RM : fmt_gen, src_10fc, dst_0rm {};

// Specials for `fsincos`
struct FMT_0RM_0FP_7FP  : fmt_gen, src_0rm,  dst_0fp, arg3_7fp {};
struct FMT_10FP_0FP_7FP : fmt_gen, src_10fp, dst_0fp, arg3_7fp {};

// Special for `fmovecr`: shift=0, bits=7, word=1
struct FMT_CR0_7FP  : fmt_gen, src_i<7, 1>, dst_7fp {};

// fmovem.x: 5 Formats: From Mem, To Mem, PreDecr, Static vs Dynamic
// only 4 are new defns.
struct FMT_0RM_I8    : fmt_gen, src_0rm, dst_i<8, 1> {};
struct FMT_I8R_0RM   : fmt_gen, src_i<8, 1, true>, dst_0rm {};
struct FMT_0RM_D4    : fmt_gen, src_0rm, dst_d4 {};
struct FMT_D4_0RM    : fmt_gen, src_d4, dst_0rm {};

// fmovem.l: 2 Formats: From Mem, To Mem
struct FMT_0RM_I13   : fmt_gen, src_0rm, dst_i<13, 1> {};
struct FMT_I13_0RM   : fmt_gen, src_i<13, 1>, dst_0rm {};


// coldfire formats
// formats for emac multiple accumulators (ACC_N)
struct FMT_9AN_0RM  : fmt_gen, src_9an, dst_0rm {};
struct FMT_0RM_9AN  : fmt_gen, src_0rm, dst_9an {};
struct FMT_0AN_9AN  : fmt_gen, src_0an, dst_9an {};

// mac MAC
struct FMT_0UL_9UL  : fmt_gen, src_0ul, dst_9ul {};

// mac MACL
struct FMT_UL0_UL12_0RM_9UL   : fmt_gen, src_ul0, dst_ul12, arg3_0rm, arg4_9ul {};
struct FMT_UL0_UL12_X_0RM_9UL : fmt_gen, src_ul0, dst_ul12, arg4_0rm, arg5_9ul {};

// emac MAC
struct FMT_0UL_9UL_7AN   : fmt_gen, src_0ul, dst_9ul, arg3_7an {};
struct FMT_0UL_9UL_X_7AN : fmt_gen, src_0ul, dst_9ul, arg4_7an {};

// emac MACL (note Accumulator LSB is inverted)
struct FMT_UL0_UL12_0RM_9UL_I7AN   : fmt_gen, src_ul0, dst_ul12, arg3_0rm, arg4_9ul, arg5_i7an {};
struct FMT_UL0_UL12_X_0RM_9UL_I7AN : fmt_gen, src_ul0, dst_ul12, arg4_0rm, arg5_9ul, arg6_i7an {};

// emac_v2 Dual ACC insns
struct FMT_0UL_9UL_7AN_AN2   : fmt_gen, src_ul0, dst_9ul, arg3_7an, arg4_an2 {};
struct FMT_0UL_9UL_X_7AN_AN2 : fmt_gen, src_ul0, dst_9ul, arg4_7an, arg5_an2 {};

}
#endif


