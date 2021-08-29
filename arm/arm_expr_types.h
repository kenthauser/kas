#ifndef KAS_ARM_ARM_EXPR_TYPES_H
#define KAS_ARM_ARM_EXPR_TYPES_H

// Define the arm terminals used in the expression subsystem
//
// These definitions allow the `expr_t` variant to be defined,
#include "expr/expr_types.h"

// need complete register definition to instantiate expr_t
#include "arm_reg_types.h"

// define error_message type
#include "arm_error_messages.h"

// expose error-type to global namespace
namespace kas
{
    using arm::error_msg;
}

// declare X3 parser for `reg_t`
namespace kas::arm::parser::bnf
{
    namespace x3 = boost::spirit::x3;

    // declare parser for ARM register tokens
    using arm_reg_x3 = x3::rule<struct X_reg, kas::parser::kas_token>;
    BOOST_SPIRIT_DECLARE(arm_reg_x3)
}

// forward declare arm types
namespace kas::arm
{
    // forward declare mcode type
    struct arm_mcode_t;

    // forward declare arm floating point formatter
    struct arm_format_float;

    // utility type: arm data is 16-bit words
    using arm_base_t = std::uint16_t;
}

namespace kas::expression::detail
{
    // ARM: define supported floating point formats
    template <> struct float_fmt<void> : meta::id<arm::arm_format_float> {};
    template <> struct err_msg  <void> : meta::id<arm::error_msg        > {};
    
    // ARM is baased on 16-bit instructions & 32-bit addresses
    template <> struct e_data   <void> : meta::id<arm::arm_base_t      > {};
    template <> struct e_addr   <void> : meta::id<std::uint32_t          > {};

    // expose terminals to expression subsystem
    // arm types for expression variant
    template <> struct term_types_v<defn_cpu> :
        meta::list<
              arm::arm_reg_t      // arm register
            , arm::arm_rs_ref     // arm register-set reference
            > {};

    // parsers for directly parsed types
    template <> struct term_parsers_v<defn_cpu> :
        meta::list<
              arm::parser::bnf::arm_reg_x3
            > {};
}


#endif
