#ifndef KAS_ARM_ARM_EXPR_TYPES_H
#define KAS_ARM_ARM_EXPR_TYPES_H

// Define the tgt terminals used in the expression subsystem
//
// These definitions allow the `expr_t` variant to be defined,

#include "expr/expr_types.h"

// need complete register definition to instantiate expr_t
#include "arm_reg_types.h"

// define error_message type
#include "arm_error_messages.h"

namespace kas::expression
{
    // override default `kas` types in `expr_types.h`
    // undefined addresses are 16-bits
    template <> struct e_data_t <void> { using type = std::uint16_t;   };
    template <> struct e_addr_t <void> { using type = std::uint16_t;  };
    template <> struct err_msg_t<void> { using type = arm::error_msg; };
}

// expose terminals to expression subsystem
namespace kas::expression::detail
{
    // types for expression variant
    template <> struct term_types_v<defn_cpu> :
        meta::list<
              arm::arm_reg_t      // arm register
            , arm::arm_rs_ref     // arm register set: index_reg + offset
            > {};

    // directly parsed types
    template <> struct term_parsers_v<defn_cpu> :
        meta::list<
              arm::parser::arm_reg_x3
            > {};
}


#endif
