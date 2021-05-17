#ifndef KAS_Z80_Z80_REG_DEFN_H
#define KAS_Z80_Z80_REG_DEFN_H

#include "z80_reg_types.h"
#include "target/tgt_reg_trait.h"

namespace kas::z80::reg_defn
{
using namespace tgt::reg_defn;

// use `REG_STR` to add prefix string if required
#define REG_STR KAS_STRING

using reg_l = meta::list<
    // 8-bit general registers
      reg<REG_STR("a"), RC_GEN, 7>
    , reg<REG_STR("b"), RC_GEN, 0>
    , reg<REG_STR("c"), RC_GEN, 1>
    , reg<REG_STR("d"), RC_GEN, 2>
    , reg<REG_STR("e"), RC_GEN, 3>
    , reg<REG_STR("h"), RC_GEN, 4>
    , reg<REG_STR("l"), RC_GEN, 5>
    // NB: (HL) has GEN value 6

    // DBL register 0-2 fixed
    , reg<REG_STR("bc"), RC_DBL, 0>
    , reg<REG_STR("de"), RC_DBL, 1>
    , reg<REG_STR("hl"), RC_DBL, 2>

    // DBL register value 3 can be SP or AF
    , reg<REG_STR("sp"), RC_SP, 3>
    , reg<REG_STR("af"), RC_AF, 3>

    // HL is also an index register
    , reg<REG_STR("hl"), RC_IDX, 0>
    , reg<REG_STR("ix"), RC_IDX, 0xdd>
    , reg<REG_STR("iy"), RC_IDX, 0xfd>

    // interrupt vector & refesh registers
    , reg<REG_STR("i") , RC_I>
    , reg<REG_STR("r") , RC_R>

    // condition codes are like registers in Zilog syntax
    , reg<REG_STR("nz"), RC_CC, 0>
    , reg<REG_STR("z") , RC_CC, 1>
    , reg<REG_STR("nc"), RC_CC, 2>
    , reg<REG_STR("c") , RC_CC, 3>
    , reg<REG_STR("po"), RC_CC, 4>
    , reg<REG_STR("pe"), RC_CC, 5>
    , reg<REG_STR("p") , RC_CC, 6>
    , reg<REG_STR("m") , RC_CC, 7>
    >;
 
// expose single register name list as "combined" list name
using z80_all_reg_l = reg_l;

// allow af-prime to be specified w/o apostrophe
using z80_reg_aliases_l = meta::list<
      meta::list<REG_STR("af"), REG_STR("af'")>
    >;
}

#undef REG_STR

#endif
