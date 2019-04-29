#ifndef KAS_ARM_ARM_REG_DEFN_H
#define KAS_ARM_ARM_REG_DEFN_H

#include "arm_reg_types.h"
#include "target/tgt_reg_trait.h"

namespace kas::arm::reg_defn
{
using namespace tgt::reg_defn;

// use `REG_STR` to add prefix if required
#define REG_STR KAS_STRING

using gen_reg_l    = make_reg_seq<reg_seq<RC_GEN>    , REG_STR("r"), 16>;
using fp_sgl_reg_l = make_reg_seq<reg_seq<RC_FLT_SGL>, REG_STR("s"), 32>;
using fp_dbl_reg_l = make_reg_seq<reg_seq<RC_FLT_DBL>, REG_STR("d"), 16>;

#if 0
// dbl 16-31 is an extension. quad 0-16 is an extension
// condition on `hw_test`
using gen_fp_dbl_l = tgt::make_reg_seq<tgt::reg_seq<RC_FLT_DBL> , REG_STR("d"), 16, hw::hw_void, 16>;
using gen_fp_dbl_l = tgt::make_reg_seq<tgt::reg_seq<RC_FLT_QUAD>, REG_STR("q"), 16, hw::hw_void>;
#endif


using cpu_reg_l = list<
       reg<REG_STR("apsr") , RC_CPU, REG_CPU_APSR , void>
     , reg<REG_STR("fpscr"), RC_CPU, REG_CPU_FPSCR, void>
     , reg<REG_STR("psr")  , RC_CPU, REG_CPU_PSR  , void>
     , reg<REG_STR("cpsr") , RC_CPU, REG_CPU_CPSR , void>
     , reg<REG_STR("spsr") , RC_CPU, REG_CPU_SPSR , void>
    >;


using reg_aliases_l = meta::list<
    // declare aliases and make alias names canonical
      list<REG_STR("r13"), REG_STR("sp")>//, std::true_type>
    , list<REG_STR("r14"), REG_STR("lr")>//, std::true_type>
    , list<REG_STR("r15"), REG_STR("pc")>//, std::true_type>
    >;

#undef REG_STR

// combine all classes into single list
using reg_l = concat<list<>
                   , gen_reg_l
                   , fp_sgl_reg_l
                   , fp_dbl_reg_l
                   , cpu_reg_l
                    >;
}
#endif
