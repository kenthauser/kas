#ifndef KAS_CORE_TYPES_H
#define KAS_CORE_TYPES_H

#include "expr/expr_types.h"
#include "core_terminal_types.h"

//#include "core_expr_type.h"
//#include "core_symbol.h"
//#include "core_addr.h"

#include "core_chunk_types.h"

#include <cstdint>
#include <iostream>

namespace kas::core
{
// alias a couple of types from ::kas::parser namespace
//using kas_pos = ::kas::parser::kas_position_tagged;
using kas_loc = ::kas::parser::kas_loc;

// forward declaration
struct insn_container_data;
}

namespace opc
{
    template <typename Container = std::deque<kas::expression::ast::expr_t>>
    struct opcode_data;// : protected Container;

    template <typename DATA = opcode_data<>>
    struct insn_opcode;
    
    // create alias using default types
    using opcode = insn_opcode<>;
}

namespace kas::expression::detail
{
// NB: in detail namespace, all types are `meta`

// all `base_types` are location tagged
using core_base_types = list<
    // value holding types
      core::expr_ref
    , core::symbol_ref
    , core::addr_ref
    >;

template <> struct term_types_v<defn_core> : concat<list<>
        , core_base_types
        , core::chunk_types
    > {};
}

#endif
