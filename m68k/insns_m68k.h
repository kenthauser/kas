#ifndef KAS_M68K_INSNS_M68K_H
#define KAS_M68K_INSNS_M68K_H
//
// Define the M68K instructions. 
//
// For information in the table format, see `target/tgt_mcode_defn_trait.h`
//
// The format names (and naming convention) are in `m68k_format_defn.h`
//
// The argument validators are in `m68k_validate.h`
 //
// The common validators with ambiguous names are:
//
// REG          : allow registers

// Convention is for formatter type names to list "shifts" for args in order. 
// shift of `X` indicates arg is not inserted in machine code.
// for shift if `X`, arg is either determined by "validator" (eg REG_R0)
// or immediate arg validators can normally determine emit size


#include "m68k_insn_common.h"

namespace kas::m68k::opc::gen
{
using namespace meta;

#define STR KAS_STRING


using m68k_insn_ld_l = list<list<>
//
// Dummy machine-code for "list" opcode
//
, defn<sz_w, STR("*LIST*"), OP<0>, FMT_LIST>//, REG, REG>

//
// EXAMPLE: dummy "ld" insn to show pattern...
//
//, defn<sz_b, STR("move"), OP<0x00>, FMT_3_0, REG, REG>
>;

using m68k_insn_list = list<list<>
                         , m68k_insn_ld_l
                         >;
}

namespace kas::m68k::opc
{
    template <> struct m68k_insn_defn_list<OP_M68K_GEN> : gen::m68k_insn_list {};
}

#undef STR

#endif

