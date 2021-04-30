#ifndef KAS_PROTO_UC_INSNS_PROTO_UC_H
#define KAS_PROTO_UC_INSNS_PROTO_UC_H
//
// Define the PROTO_UC instructions. 
//
// For information in the table format, see `target/tgt_mcode_defn_trait.h`
//
// The format names (and naming convention) are in `PROTO_LC_format_defn.h`
//
// The argument validators are in `PROTO_LC_validate.h`
 //
// The common validators with ambiguous names are:
//
// REG          : allow registers

// Convention is for formatter type names to list "shifts" for args in order. 
// shift of `X` indicates arg is not inserted in machine code.
// for shift if `X`, arg is either determined by "validator" (eg REG_R0)
// or immediate arg validators can normally determine emit size


#include "PROTO_LC_insn_common.h"

namespace kas::PROTO_LC::opc::gen
{
using namespace meta;

#define STR KAS_STRING


using PROTO_LC_insn_ld_l = list<list<>
//
// Dummy machine-code for "list" opcode
//
, defn<sz_w, STR("*LIST*"), OP<0>, FMT_LIST, REG, REG>

//
// EXAMPLE: dummy "ld" insn to show pattern...
//
, defn<sz_b, STR("move"), OP<0x00>, FMT_3_0, REG, REG>
>;

using PROTO_LC_insn_list = list<list<>
                         , PROTO_LC_insn_ld_l
                         >;
}

namespace kas::PROTO_LC::opc
{
    template <> struct PROTO_LC_insn_defn_list<OP_PROTO_UC_GEN> : gen::PROTO_LC_insn_list {};
}

#undef STR

#endif

