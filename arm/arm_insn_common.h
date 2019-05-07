#ifndef KAS_ARM_ARM_INSN_COMMON_H
#define KAS_ARM_ARM_INSN_COMMON_H

// arm instruction definion patterns
//
// see `target/tgt_mcode_defn_trait.h` for description of
// insn definition pattern.

#include "arm_format.h"             // actual format types
#include "arm_validate.h"           // actual validate types

#include "target/tgt_insn_common.h"  // declare "trait" for definition

namespace kas::arm::opc
{

// declare opcode groups (ie: include files)
using arm_insn_defn_groups = meta::list<
      struct OP_ARM_GEN
    >;

template <typename=void> struct arm_insn_defn_list : meta::list<> {};

using namespace tgt::opc::traits;
using tgt::opc::define_sz;
using tgt::opc::sz_void;

using sz_v    = sz_void;
using sz_b    = define_sz<OP_SIZE_BYTE>;
using sz_w    = define_sz<OP_SIZE_WORD>;

// 

}


#endif

