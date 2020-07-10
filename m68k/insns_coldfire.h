#ifndef KAS_M68K_INSNS_COLDFIRE_H
#define KAS_M68K_INSNS_COLDFIRE_H

#include "m68k_insn_common.h"

namespace kas::m68k::opc::ns_coldfire
{

#define STR KAS_STRING

using cf_mac_moves_v = list<list<>

// MAC + eMAC: moves
, defn<sz_l, STR("move"), OP<0xa180, coldfire>, FMT_9_0RM, MAC, GEN_REG>
, defn<sz_l, STR("move"), OP<0xa100, coldfire>, FMT_0RM_9, MAC_GEN, MAC>
    
, defn<sz_l, STR("move"), OP<0xa9c0, coldfire>, void, MACSR, CCR>
, defn<sz_l, STR("move"), OP<0xa110, emac>, FMT_0_9, ACC, ACC>

, defn<sz_l, STR("movclr"), OP<0xa1c0, emac>, FMT_9_0RM, ACC, GEN_REG>
>;

// the MAC (and eMAC) have many odd syntax quarks (eg optional << or >>)
// Also optional `&` after indirect arg to indicate MASK in use
//
// NB: using separate MASK validators allows the "mask" bit to be encoded
// via validator instead if via standard serialze methods (ie arg.mode, etc)
// Not strictly compatible with "list" format, but "list" should never be
// required for "m***l" formats which use mask, as args are never ambiguous

using cf_mac_v = list<list<>
// original mac
// MAC/MSAS without load
, defn<sz_lw, STR("mac"),  OP<0xa000'0000, mac, INFO_SIZE_MAC>
                , FMT_0UL_9UL, REG_UL, REG_UL>
, defn<sz_lw, STR("mac"),  OP<0xa000'0000, mac, INFO_SIZE_MAC>
                , FMT_0UL_9UL_SF, REG_UL, REG_UL, CF_SHIFT>

, defn<sz_lw, STR("msac"),  OP<0xa000'0100, mac, INFO_SIZE_MAC>
                , FMT_0UL_9UL, REG_UL, REG_UL>
, defn<sz_lw, STR("msac"),  OP<0xa000'0100, mac, INFO_SIZE_MAC>
                , FMT_0UL_9UL_SF, REG_UL, REG_UL, CF_SHIFT>

// MAC/MSAS with load
// nxp app notes show `l`, but programmers reference doesn't... 
// INDIR_MASK validator requires `MASK`. INDIRECT matches both with & without.
// First to match is chosen...so order matters
, defn<sz_lw, STR("macl"),  OP<0xa080'0020, mac, INFO_SIZE_MAC>
        , FMT_UL0_UL12_SF_0RM_9G,   REG_UL, REG_UL, INDIR_MASK, GEN_REG>
, defn<sz_lw, STR("macl"),  OP<0xa080'0000, mac, INFO_SIZE_MAC>
        , FMT_UL0_UL12_SF_0RM_9G,   REG_UL, REG_UL, INDIRECT, GEN_REG>
, defn<sz_lw, STR("macl"),  OP<0xa080'0020, mac, INFO_SIZE_MAC>
        , FMT_UL0_UL12_SF_0RM_9G,   REG_UL, REG_UL, CF_SHIFT, INDIR_MASK, GEN_REG>
, defn<sz_lw, STR("macl"),  OP<0xa080'0000, mac, INFO_SIZE_MAC>
        , FMT_UL0_UL12_SF_0RM_9G,   REG_UL, REG_UL, CF_SHIFT, INDIRECT, GEN_REG>

, defn<sz_lw, STR("msacl"),  OP<0xa080'0120, mac, INFO_SIZE_MAC>
        , FMT_UL0_UL12_SF_0RM_9G,   REG_UL, REG_UL, INDIR_MASK, GEN_REG>
, defn<sz_lw, STR("msacl"),  OP<0xa080'0100, mac, INFO_SIZE_MAC>
        , FMT_UL0_UL12_SF_0RM_9G,   REG_UL, REG_UL, INDIRECT, GEN_REG>
, defn<sz_lw, STR("msacl"),  OP<0xa080'0120, mac, INFO_SIZE_MAC>
        , FMT_UL0_UL12_SF_0RM_9G,   REG_UL, REG_UL, CF_SHIFT, INDIR_MASK, GEN_REG>
, defn<sz_lw, STR("msacl"),  OP<0xa080'0100, mac, INFO_SIZE_MAC>
        , FMT_UL0_UL12_SF_0RM_9G,   REG_UL, REG_UL, CF_SHIFT, INDIRECT, GEN_REG>
>;

// extended mac
using cf_emac_v = list<list<>
// eMAC MAC/MSAS without load
, defn<sz_lw, STR("mac"),  OP<0xa000'0000, emac, INFO_SIZE_MAC>
    , FMT_0UL_9UL_AN,   REG_UL, REG_UL, ACC>
, defn<sz_lw, STR("mac"),  OP<0xa000'0000, emac, INFO_SIZE_MAC>
    , FMT_0UL_9UL_SF_AN, REG_UL, REG_UL, CF_SHIFT, ACC>

, defn<sz_lw, STR("msac"),  OP<0xa000'0100, emac, INFO_SIZE_MAC>
        , FMT_0UL_9UL_AN,   REG_UL, REG_UL, ACC>
, defn<sz_lw, STR("msac"),  OP<0xa000'0300, emac, INFO_SIZE_MAC>
        , FMT_0UL_9UL_SF_AN, REG_UL, REG_UL, CF_SHIFT, ACC>
   
// MAC/MSAS with load
// nxp app notes show `l`, but programmers reference doesn't... 
// INDIR_MASK validator requires `MASK`. INDIRECT matches both with & without.
// First to match is chosen...so order matters

// accumulator_N LSB is inverted for `loads`! Wow!
, defn<sz_lw, STR("macl"),  OP<0xa000'0000, emac, INFO_SIZE_MAC>
    , FMT_UL0_UL12_0RM_9G_ANI,   REG_UL, REG_UL, INDIRECT, GEN_REG, ACC>
, defn<sz_lw, STR("macl"),  OP<0xa000'0020, emac, INFO_SIZE_MAC>
    , FMT_UL0_UL12_0RM_9G_ANI,   REG_UL, REG_UL, INDIR_MASK, GEN_REG, ACC>
, defn<sz_lw, STR("macl"),  OP<0xa000'0000, emac, INFO_SIZE_MAC>
    , FMT_UL0_UL12_SF_0RM_9G_ANI, REG_UL, REG_UL, CF_SHIFT,  INDIRECT, GEN_REG, ACC>
, defn<sz_lw, STR("macl"),  OP<0xa000'0020, emac, INFO_SIZE_MAC>
    , FMT_UL0_UL12_SF_0RM_9G_ANI, REG_UL, REG_UL, CF_SHIFT,  INDIR_MASK, GEN_REG, ACC>

, defn<sz_lw, STR("msacl"),  OP<0xa000'0100, emac, INFO_SIZE_MAC>
        , FMT_UL0_UL12_0RM_9G_ANI, REG_UL, REG_UL, INDIR_MASK, GEN_REG, ACC>
, defn<sz_lw, STR("msacl"),  OP<0xa000'0100, emac, INFO_SIZE_MAC>
        , FMT_UL0_UL12_0RM_9G_ANI, REG_UL, REG_UL, INDIRECT, GEN_REG, ACC>
, defn<sz_lw, STR("msacl"),  OP<0xa000'0100, emac, INFO_SIZE_MAC>
        , FMT_UL0_UL12_SF_0RM_9G_ANI, REG_UL, REG_UL, CF_SHIFT, INDIR_MASK, GEN_REG, ACC>
, defn<sz_lw, STR("msacl"),  OP<0xa000'0100, emac, INFO_SIZE_MAC>
        , FMT_UL0_UL12_SF_0RM_9G_ANI, REG_UL, REG_UL, CF_SHIFT, INDIRECT, GEN_REG, ACC>
>;

// eMAC rev-B insns
using cf_emac_b_v = list<list<>
, defn<sz_lw, STR("maaac"),  OP<0xa000'0001, emac_b, INFO_SIZE_MAC>
        , FMT_0UL_9UL_AN_AN2,   REG_UL, REG_UL, ACC, ACC>
, defn<sz_lw, STR("maaac"),  OP<0xa000'0001, emac_b, INFO_SIZE_MAC>
        , FMT_0UL_9UL_SF_AN_AN2, REG_UL, REG_UL, CF_SHIFT, ACC, ACC>
    
, defn<sz_lw, STR("masac"),  OP<0xa000'0003, emac_b, INFO_SIZE_MAC>
        , FMT_0UL_9UL_AN_AN2,   REG_UL, REG_UL, ACC, ACC>
, defn<sz_lw, STR("masac"),  OP<0xa000'0003, emac_b, INFO_SIZE_MAC>
        , FMT_0UL_9UL_SF_AN_AN2, REG_UL, REG_UL, CF_SHIFT, ACC, ACC>
    
, defn<sz_lw, STR("msaac"),  OP<0xa000'0101, emac_b, INFO_SIZE_MAC>
        , FMT_0UL_9UL_AN_AN2,   REG_UL, REG_UL, ACC, ACC>
, defn<sz_lw, STR("msaac"),  OP<0xa000'0101, emac_b, INFO_SIZE_MAC>
        , FMT_0UL_9UL_SF_AN_AN2, REG_UL, REG_UL, CF_SHIFT, ACC, ACC>
    
, defn<sz_lw, STR("mssac"),  OP<0xa000'0103, emac_b, INFO_SIZE_MAC>
        , FMT_0UL_9UL_AN_AN2,   REG_UL, REG_UL, ACC, ACC>
, defn<sz_lw, STR("mssac"),  OP<0xa000'0103, emac_b, INFO_SIZE_MAC>
        , FMT_0UL_9UL_SF_AN_AN2, REG_UL, REG_UL, CF_SHIFT, ACC, ACC>
>;


// generic coldfire INSNS
using cf_supv_v = list<list<>
// re-add core insns otherwise excluded
, defn<sz_vb, STR("tas"),  OP<0x4ac0, isa_c>, FMT_0RM, DATA_ALTER>
, defn<sz_vl, STR("extb"), OP<0x49c0, coldfire>, FMT_0, DATA_REG>

, defn<sz_l, STR("byterev"), OP<0x2c0, isa_c>, FMT_0, DATA_REG>

, defn<sz_l, STR("mov3q"),  OP<0xa140, isa_b>, FMT_9_0RM, Q_MOV3Q, ALTERABLE>
, defn<sz_w, STR("mvs"),    OP<0x7140, isa_b>, FMT_0RM_9, GEN, DATA_REG>
, defn<sz_b, STR("mvs"),    OP<0x7100, isa_b>, FMT_0RM_9, GEN, DATA_REG>
, defn<sz_w, STR("mvz"),    OP<0x71c0, isa_b>, FMT_0RM_9, GEN, DATA_REG>
, defn<sz_b, STR("mvz"),    OP<0x7180, isa_b>, FMT_0RM_9, GEN, DATA_REG>
, defn<sz_l, STR("sats"),   OP<0x4c80, isa_b>, FMT_0, DATA_REG>
, defn<sz_w, STR("tpf"),    OP<0x51fa, coldfire>, void, IMMED> 
, defn<sz_l, STR("tpf"),    OP<0x51fb, coldfire>, void, IMMED> 
, defn<sz_v, STR("tpf"),    OP<0x51fc, coldfire>>
    
, defn<sz_v, STR("pulse"),   OP<0x4acc, coldfire>>

// XXX REMS/REMU not supported by MCF5202/5204/5206
, defn<sz_l, STR("rems"),    OP<0x4c40'0800, isa_a>, FMT_0RM_PAIR, DATA_REG, PAIR>
, defn<sz_l, STR("rems"),    OP<0x4c40'0800, isa_a>, FMT_0RM_PAIR, INDIRECT, PAIR> 
, defn<sz_l, STR("remu"),    OP<0x4c40'0000, isa_a>, FMT_0RM_PAIR, DATA_REG, PAIR>
, defn<sz_l, STR("remu"),    OP<0x4c40'0000, isa_a>, FMT_0RM_PAIR, INDIRECT, PAIR> 

, defn<sz_v, STR("intouch"), OP<0xf428,      isa_b>, FMT_0, ADDR_INDIR>
, defn<sz_w, STR("strldsr"), OP<0x40e7'46fc, isa_c>, void,  IMMED>     // immed word

// DEBUG cp
, defn<sz_l, STR("wdebug"),  OP<0xfbc0'0003, isa_a>, FMT_0RM, ADDR_INDIR>
, defn<sz_l, STR("wdebug"),  OP<0xfbc0'0003, isa_a>, FMT_0RM, ADDR_DISP> 
, defn<sz_lwb, STR("wddata"), OP<0xfb00, isa_a, INFO_SIZE_NORM>, FMT_0RM, MEM_ALTER>
    
>; 

using cf_insn_v = list<list<>
                , cf_mac_moves_v
                , cf_mac_v
                , cf_emac_v
                , cf_emac_b_v
                , cf_supv_v
                >;

#undef STR
}

namespace kas::m68k::opc
{
    template <> struct m68k_defn_list<OP_COLDFIRE> : ns_coldfire::cf_insn_v {};
}

#endif
