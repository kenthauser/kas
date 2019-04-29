#ifndef KAS_ARM_INSNS_ARM_H
#define KAS_ARM_INSNS_ARM_H
//
// Define the ARM instructions. 
//
// For information in the table format, see `target/tgt_mcode_defn_trait.h`
//
// The format names (and naming convention) are in `arm_format_defn.h`
//
// The argument validators are in `arm_validate.h`
 //
// The common validators with ambiguous names are:
//
// REG          : allow general register
//
// Conventions is for formater type names to list "shifts" for args in order. 
// shift of `X` indicates arg is not inserted in machine code
// for shift if `X`, arg is either determined by "validator" (eg SP) or immediate
// immediate arg validators can determine emit size


#include "arm_insn_common.h"

namespace kas::arm::opc::gen
{
using namespace meta;

#define STR KAS_STRING


using arm_insn_ld_l = list<list<>
//
// Dummy machine-code for "list" opcode
//

, defn<sz_w, STR("*LIST*"), OP<0>, FMT_LIST, REG, REG>

>;

using arm_insn_list = list<list<>
                         , arm_insn_ld_l
                         >;
}

namespace kas::arm::opc
{
    template <> struct arm_insn_defn_list<OP_ARM_GEN> : gen::arm_insn_list {};
}

#undef STR

#endif

