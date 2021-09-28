#ifndef KAS_DWARF_DWARF_OPC_H
#define KAS_DWARF_DWARF_OPC_H

#include "kas/kas_string.h"
#include "kas_core/opc_fixed.h"
#include "kas_core/opc_leb.h"
#include "kas_core/core_fixed_inserter.h"

//
// Generate a stream of instructions given the `insn inserter`
//
// The `insn_inserter` requires instances of "core_insn", but impliciate
// conversion from `opcode` is provided via `core_insn` ctor.
//

namespace kas::dwarf
{


using namespace kas::core::opc;

// declare base template
template <typename NAME, typename T, typename OP> struct ARG_defn;

#define DW_EMIT_OP(NAME, ...) using NAME = ARG_defn<KAS_STRING(#NAME), __VA_ARGS__>

DW_EMIT_OP(UBYTE, uint8_t     , opc_fixed<uint8_t>);
DW_EMIT_OP(UHALF, uint16_t    , opc_fixed<uint16_t>);
DW_EMIT_OP(UWORD, uint32_t    , opc_fixed<uint32_t>);

DW_EMIT_OP(BYTE , int8_t      , opc_fixed<int8_t>);
DW_EMIT_OP(WORD , int16_t     , opc_fixed<int32_t>);
DW_EMIT_OP(LONG , int32_t     , opc_fixed<int64_t>);

DW_EMIT_OP(ADDR , uint32_t    , opc_fixed<uint32_t>);
DW_EMIT_OP(NAME , const char *, opc_string<std::true_type>);

DW_EMIT_OP(ULEB , uintmax_t   , opc_uleb128);
DW_EMIT_OP(SLEB , intmax_t    , opc_sleb128);

using TEXT  = NAME;     // alias
using BLOCK = LONG;     // XXX made up

#undef DW_EMIT_OP

// name all `ops`. Facilitate index->op
using dwarf_arg_ops = meta::list<
      UBYTE
    , UHALF
    , UWORD
    , BYTE
    , WORD
    , LONG
    , ADDR
    , NAME
    , TEXT
    , ULEB
    , SLEB
    , BLOCK
    >;
}

#endif
