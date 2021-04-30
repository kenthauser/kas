#ifndef KAS_PROTO_UC_PROTO_UC_REG_DEFN_H
#define KAS_PROTO_UC_PROTO_UC_REG_DEFN_H

#include "PROTO_LC_reg_types.h"
#include "target/tgt_reg_trait.h"

namespace kas::PROTO_LC::reg_defn
{
using namespace tgt::reg_defn;

// use `REG_STR` to add prefix string if required
#define REG_STR KAS_STRING

//
// EXAMPLES of `make_reg_seq`, `reg_l`, `alias`
// 

// EXAMPLE: create sequence of registers: r0-r7
using gen_reg_l = make_reg_seq<reg_seq<RC_GEN>, REG_STR("r"), 8>;

// EXAMPLE: create list of registers (example: CPU:0 named "pc")
using reg_l = meta::list<
      reg<REG_STR("pc"), RC_CPU, 0>
    >;

// EXAMPLE: allow R7 to be addressed as SP
using PROTO_LC_reg_aliases_l = meta::list<
                          list<REG_STR("r7"), REG_STR("sp")>
                        >;

// combine register definitions into single list
using PROTO_LC_all_reg_l = concat<list<>
                            , gen_reg_l
                            , reg_l
                            >;
}

#undef REG_STR

#endif
