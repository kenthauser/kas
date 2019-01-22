#ifndef KAS_Z80_TYPES_H
#define KAS_Z80_TYPES_H

// Define the z80 terminals used in the expression subsystem
//
// These definitions allow the `expr_t` variant to be defined,

#include "expr/expr_types.h"

// need complete register definition to instantiate expr_t
#include "z80_reg_types.h"

// define error_message type
#include "z80_error_messages.h"

namespace kas::expression
{
    // override default `kas` types in `expr_types.h`
    // undefined addresses are 16-bits
    template<> struct e_addr_t <void> { using type = std::uint16_t;  };
    template<> struct err_msg_t<void> { using type = z80::error_msg; };
}

// expose terminals to expression subsystem
namespace kas::expression::detail
{
    // types for expression variant
    template <> struct term_types_v<defn_cpu> :
        meta::list<
              z80::z80_reg_t      // z80 register
            , z80::z80_rs_ref     // z80 register set: index_reg + offset
            > {};

    // directly parsed types
    template <> struct term_parsers_v<defn_cpu> :
        meta::list<
              z80::parser::z80_reg_parser_p
            > {};
}


#endif
