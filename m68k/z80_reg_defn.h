#ifndef KAS_Z80_REG_DEFN_H
#define KAS_Z80_REG_DEFN_H

#include "kas/kas_string.h"

namespace kas::z80
{

template <typename NAME, unsigned REG, unsigned NUM = 0, typename TST = void>
using reg = list<NAME
               , std::integral_constant<unsigned, REG>
               , std::integral_constant<unsigned, NUM>
               , std::conditional_t<std::is_void_v<TST>, hw::hw_void, TST>
               >;

#define REG_STR KAS_STRING

enum { RC_GEN, RC_DBL, RC_IDX, RC_AF, RC_I, RC_R, RC_CC };

using reg_l = list<
    reg<REG_STR("a"), RC_GEN, 7>
    reg<REG_STR("b"), RC_GEN, 0>
    reg<REG_STR("c"), RC_GEN, 1>
    reg<REG_STR("d"), RC_GEN, 2>
    reg<REG_STR("e"), RC_GEN, 3>
    reg<REG_STR("h"), RC_GEN, 4>
    reg<REG_STR("l"), RC_GEN, 5>
    // NB: (HL) has GEN value 6

    reg<REG_STR("bc"), RC_DBL, 0>
    reg<REG_STR("de"), RC_DBL, 1>
    reg<REG_STR("hl"), RC_DBL, 2>
    reg<REG_STR("sp"), RC_DBL, 3>

    reg<REG_STR("ix"), RC_IDX, 0xdd>
    reg<REG_STR("iy"), RC_IDX, 0xfd>

    reg<REG_STR("af"), RC_AF>
    reg<REG_STR("i") , RC_I>
    reg<reg_STR("r") , RC_R>

    // condition codes are like registers in Zilog syntax
    reg<REG_STR("nz"), RC_CC, 0>
    reg<REG_STR("z") , RC_CC, 1>
    reg<REG_STR("nc"), RC_CC, 2>
    reg<REG_STR("c") , RC_CC, 3>
    reg<REG_STR("po"), RC_CC, 4>
    reg<REG_STR("pe"), RC_CC, 5>
    reg<REG_STR("p") , RC_CC, 6>
    reg<REG_STR("m") , RC_CC, 7>

}

using reg_aliases_l = list<
      list<REG_STR("af"), REG_STR("af'")>
    >;

#undef REG_STR

#endif
