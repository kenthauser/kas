#ifndef KAS_PROTO_UC_PROTO_UC_EXPR_TYPES_H
#define KAS_PROTO_UC_PROTO_UC_EXPR_TYPES_H

// Define the PROTO_LC terminals used in the expression subsystem
//
// These definitions allow the `expr_t` variant to be defined,
#include "expr/expr_types.h"

// need complete register definition to instantiate expr_t
#include "PROTO_LC_reg_types.h"

// define error_message type
#include "PROTO_LC_error_messages.h"

// expose error-type to global namespace
namespace kas
{
    using PROTO_LC::error_msg;
}

// declare X3 parser for `reg_t`
namespace kas::PROTO_LC::parser
{
    namespace x3 = boost::spirit::x3;

    // declare parser for PROTO_UC register tokens
    using PROTO_LC_reg_x3 = x3::rule<struct X_reg, kas::parser::kas_token>;
    BOOST_SPIRIT_DECLARE(PROTO_LC_reg_x3)
}

// forward declare PROTO_LC types
namespace kas::PROTO_LC
{
    // forward declare mcode type
    struct PROTO_LC_mcode_t;

    // forward declare PROTO_LC floating point formatter
    struct PROTO_LC_format_float;

    // utility type: PROTO_LC data is 16-bit words
    using PROTO_LC_base_t = std::uint16_t;
}

namespace kas::expression::detail
{
    // PROTO_UC: define supported floating point formats
    template <> struct float_fmt<void> : meta::id<PROTO_LC::PROTO_LC_format_float> {};
    template <> struct err_msg  <void> : meta::id<PROTO_LC::error_msg        > {};
    
    // PROTO_UC is baased on 16-bit instructions & 32-bit addresses
    template <> struct e_data   <void> : meta::id<PROTO_LC::PROTO_LC_base_t      > {};
    template <> struct e_addr   <void> : meta::id<std::uint32_t          > {};

    // expose terminals to expression subsystem
    // PROTO_LC types for expression variant
    template <> struct term_types_v<defn_cpu> :
        meta::list<
              PROTO_LC::PROTO_LC_reg_t      // PROTO_LC register
            , PROTO_LC::PROTO_LC_rs_ref     // PROTO_LC register-set reference
            > {};

    // parsers for directly parsed types
    template <> struct term_parsers_v<defn_cpu> :
        meta::list<
              PROTO_LC::parser::PROTO_LC_reg_x3
            > {};
}


#endif
