#ifndef KAS_PROTO_UC_PROTO_UC_INSN_COMMON_H
#define KAS_PROTO_UC_PROTO_UC_INSN_COMMON_H

// PROTO_LC instruction definion patterns
//
// see `target/tgt_insn_common.h` for description of
// insn definition pattern.

#include "PROTO_LC_formats.h"            // actual format types
#include "PROTO_LC_validate.h"           // actual validate types

#include "target/tgt_insn_common.h"  // declare "trait" for definition

namespace kas::PROTO_LC::opc
{

// declare opcode groups (ie: include files)
using PROTO_LC_insn_defn_groups = meta::list<
      struct OP_PROTO_UC_GEN
    >;

template <typename=void> struct PROTO_LC_insn_defn_list : meta::list<> {};

// EXAMPLE: define `sz` types for first arg of `defn<>` template.
//          `sz_void` required. All others are per-arch
using namespace tgt::opc::traits;
using sz_void   = meta::int_<0>;
using sz_v      = sz_void;
using sz_b      = meta::int_<OP_SIZE_BYTE>;
using sz_w      = meta::int_<OP_SIZE_WORD>;
}


#endif

