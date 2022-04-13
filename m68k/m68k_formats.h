#ifndef KAS_M68K_M68K_FORMATS_H
#define KAS_M68K_M68K_FORMATS_H


// Generate the "formatters" used to insert/extract arguments
// into the `machine-code` binary.
//
// The `formatters` are named as follows:
//
// 1. The name "base" (usually FMT_) indicates the "machine_code" virtual type to use.
//    (these are named in `formats_opc`, et.al.)
//
// 2. Following the "base", underscores are used to separate inserter for each argument.
//    Thus `FMT_a1_a2_a3` inserts three arguments, coded `a1`, `a2`, and `a3`.
//
// 3. For the M68K, most insertions are 3-bits register numbers, or 6-bit register/mode
//    combinations. For register args, the convention is to use 0-15 to represent shifts in
//    the first word, and 16-31 to represent shifts in the second word. For the 6-bit
//    variants, the convention is to add `rm` to the shift, yielding eg: `0rm`, `28rm`
//
// 4. Inserters which aren't regular use irregular naming. eg: coldfire ACCx uses
//    `9b2` for a two-bit field shifted 9. '020 CAS2 uses `CAS2` for a one-off formater.


#include "m68k_formats_opc.h"       // generic target definitions
#include "m68k_formats_defn.h"      // target specific methods

namespace kas::m68k::opc
{
// declare mixin classes to override virtual functions
// override insert/extract pairs together...

// associate format function with source/dest argument
// 3-bit reg# stored in four places
template <int SHIFT, int WORD = 0>
using fmt_reg = fmt_generic<SHIFT, 3, WORD>;

using arg1_0      = fmt_arg<1, fmt_reg<0>>;
using arg2_0      = fmt_arg<2, fmt_reg<0>>;

using arg2_3      = fmt_arg<2, fmt_reg<3>>;     // special for `LIST`
using arg3_6      = fmt_arg<3, fmt_reg<6>>;     // special for `LIST`

using arg1_9      = fmt_arg<1, fmt_reg<9>>;
using arg2_9      = fmt_arg<2, fmt_reg<9>>;

using arg1_1w0    = fmt_arg<1, fmt_reg<0, 1>>;
using arg2_1w0    = fmt_arg<2, fmt_reg<0, 1>>;

using arg1_1w4    = fmt_arg<1, fmt_reg<4, 1>>;
using arg2_1w4    = fmt_arg<2, fmt_reg<4, 1>>;

// special word1 formats for individual insns
using arg2_1w6    = fmt_arg<2, fmt_reg< 6, 1>>;
using arg2_1w12   = fmt_arg<2, fmt_reg<12, 1>>;
using arg3_1w0    = fmt_arg<3, fmt_reg< 0, 1>>;

// floating point co-processor puts reg's in several places
using arg1_1w0    = fmt_arg<1, fmt_reg< 0, 1>>;

using arg1_1w7    = fmt_arg<1, fmt_reg< 7, 1>>;
using arg2_1w7    = fmt_arg<2, fmt_reg< 7, 1>>;
using arg3_1w7    = fmt_arg<3, fmt_reg< 7, 1>>;

using arg1_1w10   = fmt_arg<1, fmt_reg<10, 1>>;
using arg2_1w10   = fmt_arg<2, fmt_reg<10, 1>>;

// 6-bit reg/mode stored two places
using arg1_0rm    = fmt_arg<1, fmt_reg_mode<0>>;
using arg2_0rm    = fmt_arg<2, fmt_reg_mode<0>>;
using arg3_0rm    = fmt_arg<3, fmt_reg_mode<0>>;
using arg4_0rm    = fmt_arg<4, fmt_reg_mode<0>>;

using arg1_1w12rm = fmt_arg<1, fmt_reg_mode<12, 1, 3, 1>>;
using arg2_1w12rm = fmt_arg<2, fmt_reg_mode<12, 1, 3, 1>>;
using arg3_1w12rm = fmt_arg<3, fmt_reg_mode<12, 1, 3, 1>>;

// special 6-bit reg/mode for `move` dest
using arg2_6rm    = fmt_arg<2, fmt_reg_mode<9, 0, -3>>;

// special for `list` format: store general register
using arg3_12rm   = fmt_arg<3, fmt_reg_mode<12, 0, 3, 1>>;


// quick immediate args: always LSBs, but varying widths
template <int N, int W = 0, bool REVERSE = false>
using arg1_i   = fmt_arg<1, fmt_generic<0, N, W>>;

template <int N, int W = 0, bool REVRSE = false>
using arg2_i   = fmt_arg<2, fmt_generic<0, N, W>>;

// bitfields are completely regular. 12 lsbs in second word
using arg2_bf  = fmt_arg<2, fmt_generic<0, 12, 1>>;
using arg3_bf  = fmt_arg<3, fmt_generic<0, 12, 1>>;

// coldfire: emac ACCn is 2 bits shifted N, first word
using arg1_0b2 = fmt_arg<1, fmt_generic<0, 2>>;
using arg1_9b2 = fmt_arg<1, fmt_generic<9, 2>>;
using arg2_9b2 = fmt_arg<2, fmt_generic<9, 2>>;

// coldfile: MAC subregister modes: bits spread all over the place...
// NB: `arg1_0g`  stores "general register" in word 0
//     `arg1_0ul` stores "general register" in word 0 and subword bit in word 1
//     `arg1_1w0ul` stores both general register & subword bit in word 1
using arg1_0g     = fmt_arg<1, fmt_subreg<0,  3>>;
using arg1_0ul    = fmt_arg<1, fmt_subreg<0,  3, 6, 0, 1>>;
using arg1_1w0ul  = fmt_arg<1, fmt_subreg<0,  3, 6, 1, 1>>;

using arg2_9g     = fmt_arg<2, fmt_subreg<9, -3>>;
using arg2_9ul    = fmt_arg<2, fmt_subreg<9, -3, 7, 0, 1>>;
using arg2_1w12ul = fmt_arg<2, fmt_subreg<12, 3, 7, 1, 1>>;

// general register (no subword)
using arg4_9g     = fmt_arg<4, fmt_subreg<9, -3>>;
using arg5_9g     = fmt_arg<5, fmt_subreg<9, -3>>;

// coldfire: MAC scale-factor (ie <<, >>)
using arg3_1sf9   = fmt_arg<3, fmt_generic<9, 2, 1>>;

// coldfire: emac ACCn has special formatter
// NB: `ani` formats: LSB of ACCn is inverted in first word (really!)
using arg3_an   = fmt_arg<3, fmt_emac_an<false>>;
using arg4_an   = fmt_arg<4, fmt_emac_an<false>>;
using arg5_an   = fmt_arg<5, fmt_emac_an<false>>;
using arg6_an   = fmt_arg<6, fmt_emac_an<false>>;

using arg3_ani  = fmt_arg<3, fmt_emac_an<true>>;
using arg4_ani  = fmt_arg<4, fmt_emac_an<true>>;
using arg5_ani  = fmt_arg<5, fmt_emac_an<true>>;
using arg6_ani  = fmt_arg<6, fmt_emac_an<true>>;

// second ACCn for `maaac`, etc is two consecutive bits
using arg4_an2  = fmt_arg<4, fmt_generic<2, 2, 1>>;
using arg5_an2  = fmt_arg<5, fmt_generic<2, 2, 1>>;
using arg6_an2  = fmt_arg<6, fmt_generic<2, 2, 1>>;

///////////////////////////////////////////////////////////////////////////


// used by OPC_LIST instructions
// NB: use 6 LSBs to store three reg args.
// NB: use next 6 bits to store ccode info (normal/float)
// NB: use 4 MSBs to store width
struct FMT_LIST     : fmt_list, arg1_0, arg2_3 {};


// branch formats have implied argument format
struct FMT_BRANCH   : fmt_branch, fmt_arg<1, fmt_displacement> {};

// {cp}dbcc have single argument
struct FMT_DBCC     : fmt_dbcc,    arg1_0 {};

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
struct FMT_0RM_28RM : fmt_gen, arg1_0rm, arg2_1w12rm {};
struct FMT_28RM_0RM : fmt_gen, arg2_0rm, arg1_1w12rm {};
struct FMT_I12_28RM : fmt_gen, arg1_i<12, 1>, arg2_1w12rm {};
struct FMT_28RM_I12 : fmt_gen, arg2_i<12, 1>, arg1_1w12rm {};

// Quick Immed 1-word formats
struct FMT_3I       : fmt_gen, arg1_i<3> {};
struct FMT_4I       : fmt_gen, arg1_i<4> {};
struct FMT_8I_9     : fmt_gen, arg1_i<8>, arg2_9 {};
struct FMT_Z_0RM    : fmt_gen, arg1_i<0>, arg2_0rm {};

// Quick Immed 2-word formats
struct FMT_0RM_I16  : fmt_gen, arg1_0rm, arg2_i<16, 1> {};
struct FMT_I16_0RM  : fmt_gen, arg1_i<16, 1>, arg2_0rm {};
struct FMT_I8_0RM   : fmt_gen, arg1_i<8, 1>, arg2_0rm {};
struct FMT_I16_9    : fmt_gen, arg1_i<16, 1>, arg2_9 {};

// special for STOP
struct FMT_I16      : fmt_gen, arg1_i<16, 1> {};
// XXX 32-bit fails compile
struct FMT_I32      : fmt_gen, arg1_i<16, 1> {};

// multiply/divide data-reg pair
struct FMT_0RM_PAIR : fmt_gen, arg1_0rm, fmt_arg<2, fmt_reg_pair<0, 12, 3>> {};

// 020 bitfield instructions
struct FMT_0RM_BF       : fmt_gen, arg1_0rm, arg2_bf {};
struct FMT_0RM_BF_28RM  : fmt_gen, arg1_0rm, arg2_bf, arg3_1w12rm {}; 
struct FMT_28RM_0RM_BF  : fmt_gen, arg1_1w12rm, arg2_0rm, arg3_bf {};

// specials for `CAS`, `CAS2`
struct FMT_16_22_0RM    : fmt_gen, arg1_1w0, arg2_1w6, arg3_0rm {};

// CAS2 is only 48-bit insn. define all inserters directly (and unnamed)
struct FMT_CAS2         : fmt_gen, fmt_arg<1, fmt_reg_pair< 0,  0, 3, 1, 2>>
                                 , fmt_arg<2, fmt_reg_pair< 6,  6, 3, 1, 2>>
                                 , fmt_arg<3, fmt_reg_pair<12, 12, 4, 1, 2>>
                                 {};

// special for `TBLxx`
struct FMT_0RM_28       : fmt_gen, arg1_0rm, arg2_1w12 {};
struct FMT_TBLPAIR_28   : fmt_gen, fmt_arg<1, fmt_reg_pair<0, 0, 3, 0, 1>>
                                 , arg2_1w12 {};

// special for `MOVE16`
struct FMT_0_28         : fmt_gen, arg1_0, arg2_1w12 {};

// coldfire: emac ACC_N formats
struct FMT_9B2_0RM      : fmt_gen, arg1_9b2, arg2_0rm {};
struct FMT_0RM_9B2      : fmt_gen, arg1_0rm, arg2_9b2 {};
struct FMT_0B2_9B2      : fmt_gen, arg1_0b2, arg2_9b2 {};


// coldfire: mac formats
struct FMT_0UL_9UL               : fmt_gen, arg1_0ul, arg2_9ul {};
struct FMT_0UL_9UL_SF            : FMT_0UL_9UL, arg3_1sf9 {};           // above + SF
struct FMT_UL0_UL12_0RM_9G       : fmt_gen, arg1_1w0ul, arg2_1w12ul, arg3_0rm, arg4_9g{};
struct FMT_UL0_UL12_SF_0RM_9G    : fmt_gen, arg1_1w0ul, arg2_1w12ul, arg4_0rm, arg5_9g
                                        , arg3_1sf9 {};

// coldfire: emac formats
#if 0
struct FMT_0UL_9UL_ANI           : fmt_gen, arg1_0ul, arg2_9ul, arg3_ani {};
struct FMT_0UL_9UL_X_ANI         : fmt_gen, arg1_0ul, arg2_9ul, arg4_ani {};
struct FMT_0UL_9UL_AN            : fmt_gen, arg1_0ul, arg2_9ul, arg3_an  {};
struct FMT_0UL_9UL_X_AN          : fmt_gen, arg1_0ul, arg2_9ul, arg4_an  {};

struct FMT_UL0_UL12_0RM_9UL_AN   : fmt_gen, arg1_1w0ul, arg2_1w12ul, arg3_0rm, arg4_9g, arg5_an {};
struct FMT_UL0_UL12_X_0RM_9UL_AN : fmt_gen, arg1_1w0ul, arg2_1w12ul, arg4_0rm, arg5_9g, arg6_an {};

struct FMT_0UL_9UL_AN_AN2        : fmt_gen, arg1_0ul, arg2_9ul, arg3_an, arg4_an2 {};
struct FMT_0UL_9UL_X_AN_AN2      : fmt_gen, arg1_0ul, arg2_9ul, arg4_an, arg5_an2 {};
#endif
// XXX
struct FMT_0UL_9UL_AN            : fmt_gen, arg1_0ul, arg2_9ul, arg3_an {};
struct FMT_0UL_9UL_SF_AN         : fmt_gen, arg1_0ul, arg2_9ul, arg3_1sf9, arg4_an {};
struct FMT_UL0_UL12_0RM_9G_ANI : fmt_gen, arg1_1w0ul, arg2_1w12ul, arg3_0rm
                                        , arg4_9g, arg5_ani {};
struct FMT_UL0_UL12_SF_0RM_9G_ANI : fmt_gen, arg1_1w0ul, arg2_1w12ul, arg3_1sf9, arg4_0rm
                                        , arg5_9g, arg6_ani {};

// coldfire: emac-b formats
struct FMT_0UL_9UL_AN_AN2        : fmt_gen, arg1_0ul, arg2_9ul, arg3_an, arg4_an2 {};
struct FMT_0UL_9UL_SF_AN_AN2     : fmt_gen, arg1_0ul, arg2_9ul, arg4_an, arg5_an2
                                            , arg3_1sf9 {};

// Floating point formats
struct FMT_0RM_20                : fmt_gen, arg1_0rm,  arg2_1w4 {};
struct FMT_0RM_23                : fmt_gen, arg1_0rm,  arg2_1w7 {};
struct FMT_0RM_26                : fmt_gen, arg1_0rm,  arg2_1w10 {};
struct FMT_0RM_16_23             : fmt_gen, arg1_0rm,  arg2_1w0, arg3_1w7 {};
struct FMT_26_16_23              : fmt_gen, arg1_1w10, arg2_1w0, arg3_1w7 {};
struct FMT_26                    : fmt_gen, arg1_1w10 {};
struct FMT_26_23                 : fmt_gen, arg1_1w10, arg2_1w7 {};
struct FMT_20_0RM                : fmt_gen, arg1_1w7 , arg2_0rm {};
struct FMT_26_0RM                : fmt_gen, arg1_1w10, arg2_0rm {};
struct FMT_DUPL_26_23            : fmt_gen, arg1_1w10 {};   // XXX need DUPL
struct FMT_I7_23                 : fmt_gen, arg1_i<7, 1>, arg2_1w7 {}; 

struct FMT_0RM_I8                : fmt_gen, arg1_0rm, arg2_i<8, 1> {};

}
#endif


