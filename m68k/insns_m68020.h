#ifndef KAS_M68K_M68020_DEFNS_H
#define KAS_M68K_M68020_DEFNS_H

// new instructions for the '020 processor
// bitfields are a little special because also supported by coldfire

#include "m68k_insn_common.h"

namespace kas::m68k::opc::gen_020
{
//using namespace common;

#define STR KAS_STRING

///////////////////////////////////////////////////////////////////////////////
//
// ALU support "metafunctions" (actually aliases)
// support families of instructions with minimal typing
//
///////////////////////////////////////////////////////////////////////////////

using m68k_math_v = list<list<>
// multiply & divide (68020 & cpu32 forms)
, defn<sz_vl, STR("muls"), OP<0x4c00'0800, mult64>, FMT_0RM_28RM, DATA, DATA_REG> 
, defn<sz_vl, STR("muls"), OP<0x4c00'0c00, mult64>, FMT_0RM_PAIR, DATA, PAIR>     
, defn<sz_vl, STR("mulu"), OP<0x4c00'0000, mult64>, FMT_0RM_28RM, DATA, DATA_REG> 
, defn<sz_vl, STR("mulu"), OP<0x4c00'0c00, mult64>, FMT_0RM_PAIR, DATA, PAIR>     

, defn<sz_vl, STR("divs"),  OP<0x4c40'0800, mult64>, FMT_0RM_PAIR, DATA, DATA_REG> 
, defn<sz_vl, STR("divs"),  OP<0x4c40'0c00, mult64>, FMT_0RM_PAIR, DATA, PAIR>     
, defn<sz_vl, STR("divsl"), OP<0x4c40'0800, mult64>, FMT_0RM_PAIR, DATA, PAIR>     
, defn<sz_vl, STR("divu"),  OP<0x4c40'0000, mult64>, FMT_0RM_PAIR, DATA, DATA_REG>  
, defn<sz_vl, STR("divu"),  OP<0x4c40'0400, mult64>, FMT_0RM_PAIR, DATA, PAIR>     
, defn<sz_vl, STR("divul"), OP<0x4c40'0000, mult64>, FMT_0RM_PAIR, DATA, PAIR>     

// misc alu math instructions
, defn<sz_v,  STR("pack"), OP<0x8148, m68020>, FMT_0_9, PRE_DECR, PRE_DECR, IMMED>
, defn<sz_v,  STR("pack"), OP<0x8140, m68020>, FMT_0_9, DATA_REG, DATA_REG, IMMED>
, defn<sz_v,  STR("unpk"), OP<0x8188, m68020>, FMT_0_9, PRE_DECR, PRE_DECR, IMMED>
, defn<sz_v,  STR("unpk"), OP<0x8180, m68020>, FMT_0_9, DATA_REG, DATA_REG, IMMED>
>;

///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////

// pull in condition-code support for `gen` (aka 68000) namespace
using gen::cc;
using gen::cc_sz;
using gen::all_cc_names;

using m68k_branch_cc_v = list<list<>
// trapcc
, cc<0x50fc, all_cc_names, STR("trap"), m68020>
, cc_sz<0x50fa, sz_lw, INFO_SIZE_WL0, all_cc_names, STR("trap"), m68020, void, IMMED>

// other branch/returns
, defn<sz_v, STR("callm"), OP<0x06c0'0000, callm>, FMT_I8_0RM, Q_8BITS, CONTROL>
, defn<sz_v, STR("rtm"), OP<0x06c0, callm>, FMT_0RM, GEN_REG>
>;

// bitfields are very regular
template <typename NAME, uint32_t N, typename...Args>
using bf = defn<sz_v, string::str_cat<STR("bf"), NAME>
                , OP<(0xe8c0 + (N << 8)) << 16, m68020>
                , Args...
                >;

using m68k_misc_v = list<list<>
, defn<sz_lwb, STR("cmp2"), OP<0x00c0'0000, m68020>, FMT_0RM_28RM, CONTROL, GEN_REG> 
, defn<sz_lwb, STR("chk2"), OP<0x00c0'0800, m68020>, FMT_0RM_28RM, ALTERABLE, GEN_REG> 
, defn<sz_vl,  STR("extb"), OP<0x49c0, m68020>, FMT_0, DATA_REG>
, defn<sz_l,   STR("link"), OP<0x4808, m68020>, FMT_0, ADDR_REG, IMMED>

, defn<sz_lwb, STR("cas"),  OP<0x0860'0000, m68020, INFO_SIZE_BWL9>, FMT_16_22_0RM, DATA_REG, DATA_REG, MEM_ALTER>
, defn<sz_lw,  STR("cas2"), OP<0x0cfc     , m68020, INFO_SIZE_WL9> , FMT_CAS2, PAIR, PAIR, GEN_PAIR>


// bitfield instructions are regular.
// all have DATA_REG & CONTROL(,_ALTER} forms...
, bf<STR("tst"),  0, FMT_0RM_BF,      DATA_REG, BITFIELD>
, bf<STR("tst"),  0, FMT_0RM_BF,      CONTROL,  BITFIELD>

, bf<STR("extu"), 1, FMT_0RM_BF_28RM, DATA_REG, BITFIELD, DATA_REG>
, bf<STR("extu"), 1, FMT_0RM_BF_28RM, CONTROL,  BITFIELD, DATA_REG>
    
, bf<STR("chg"),  2, FMT_0RM_BF,      DATA_REG,      BITFIELD>
, bf<STR("chg"),  2, FMT_0RM_BF,      CONTROL_ALTER, BITFIELD>
    
, bf<STR("exts"), 3, FMT_0RM_BF_28RM, DATA_REG, BITFIELD, DATA_REG>
, bf<STR("exts"), 3, FMT_0RM_BF_28RM, CONTROL,  BITFIELD, DATA_REG>
    
, bf<STR("clr"),  4, FMT_0RM_BF,      DATA_REG,      BITFIELD>
, bf<STR("clr"),  4, FMT_0RM_BF,      CONTROL_ALTER, BITFIELD>
    
, bf<STR("ffo"),  5, FMT_0RM_BF_28RM, DATA_REG, BITFIELD, DATA_REG>
, bf<STR("ffo"),  5, FMT_0RM_BF_28RM, CONTROL,  BITFIELD, DATA_REG>
    
, bf<STR("set"),  6, FMT_0RM_BF,      DATA_REG,      BITFIELD>
, bf<STR("set"),  6, FMT_0RM_BF,      CONTROL_ALTER, BITFIELD>
    
, bf<STR("ins"),  7, FMT_28RM_0RM_BF, DATA_REG, DATA_REG,      BITFIELD>
, bf<STR("ins"),  7, FMT_28RM_0RM_BF, DATA_REG, CONTROL_ALTER, BITFIELD>
>;

#undef STR

using m68k_gen_v = list<list<>
                  , m68k_math_v
                  , m68k_branch_cc_v
                  , m68k_misc_v
              >;
}

namespace kas::m68k::opc
{
    template <> struct m68k_defn_list<OP_M68K_020> : gen_020::m68k_gen_v {};
}

#endif
