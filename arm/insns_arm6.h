#ifndef KAS_ARM_INSNS_ARM6_H
#define KAS_ARM_INSNS_ARM6_H
//
// Define the ARM instructions. 
//
// For information in the table format, see `target/tgt_mcode_defn_trait.h`
//
// The format names (and naming convention) are in `arm_formats.h`
// The argument validators are in `arm_validate.h`
//
// The common validators with ambiguous names are:
//
// REG          : allow registers

// Convention is for formatter type names to list "shifts" for args in order. 
// shift of `X` indicates arg is not inserted in machine code.
// for shift if `X`, arg is either determined by "validator" (eg REG_R0)
// or immediate arg validators can normally determine emit size
 
 
#include "arm_insn_common.h"


namespace kas::arm::opc::arm7
{
#define STR KAS_STRING


#undef STR

using arm_gen6_v =
             list<list<>
//                 , arm_insn_common_l        // prefered mappings: eg ld <reg>, #0 -> clr <reg>
//                 , arm_insn_data_l          // A5.1: data insns 
//                 , arm_insn_load_store_l    // A5.2: load/store
//                 , arm_insn_ls_misc_l       // A5.3: load/store misc
//                 , arm_insn_ls_multiple_l   // A5.4: load/store misc
                 
//                 , arm_insn_branch_l        // A3.3: branch & related
//                 , arm_insn_multiply_l      // A3.5 Multiply & Multiply Accumulate
#if 0
                 , arm_insn_media_l         // A5.4: media insns
                 , arm_insn_cp_supv_l       // A5.6: co-processor and supervisor
                 , arm_insn_uncond_l        // A5.7: unconditional insns
#endif
                 >;
}

// boilerplate to locate ARM6 insns
namespace kas::arm::opc
{
    template <> struct arm_insn_defn_list<OP_ARM_ARM6> : arm7::arm6_gen_v {};
}

#endif
