#ifndef KAS_Z80_Z80_EXPR_TYPES_H
#define KAS_Z80_Z80_EXPR_TYPES_H

// Define the z80 terminals used in the expression subsystem
//
// These definitions allow the `expr_t` variant to be defined,
#include "expr/expr_types.h"

// need complete register definition to instantiate expr_t
#include "z80_reg_types.h"

// define error_message type
#include "z80_error_messages.h"

// expose error-type to global namespace
namespace kas
{
    using z80::error_msg;
}

// declare X3 parser for `reg_t`
namespace kas::z80::parser
{
    namespace x3 = boost::spirit::x3;

    // declare parser for Z80 register tokens
    using z80_reg_x3 = x3::rule<struct X_reg, kas::parser::kas_token>;
    BOOST_SPIRIT_DECLARE(z80_reg_x3)
}

// forward declare z80 types
namespace kas::z80
{
    // forward declare mcode type
    struct z80_mcode_t;

    // utility type: z80 data is 8-bit words
    using z80_base_t = std::uint8_t;
}

namespace kas::expression::detail
{
    // override default `kas` types in `expr_types.h`

    // Z80 is based in 8-bit instructions and 16-bit addresses
    template <> struct e_data <void> : meta::id<z80::z80_base_t> {};
    template <> struct e_addr <void> : meta::id<std::uint16_t  > {};
    template <> struct err_msg<void> : meta::id<z80::error_msg > {};

    // floating point not supported
    //template <> struct float_value<void> : meta::id<void> {};

    // expose terminals to expression subsystem
    // z80 types for expression variant
    template <> struct term_types_v<defn_cpu> :
        meta::list<
              z80::z80_reg_t      // z80 register
            , z80::z80_rs_ref     // z80 register-set reference
            > {};

    // parsers for directly parsed types
    template <> struct term_parsers_v<defn_cpu> :
        meta::list<
              z80::parser::z80_reg_x3
            > {};
}


#endif
