#ifndef KAS_Z80_Z80_EXPR_TYPES_H
#define KAS_Z80_Z80_EXPR_TYPES_H

// Define the tgt terminals used in the expression subsystem
//
// These definitions allow the `expr_t` variant to be defined,

#include "expr/expr_types.h"

// need complete register definition to instantiate expr_t
#include "z80_reg_types.h"

// define error_message type
#include "z80_error_messages.h"

// declare X3 parser for `reg_t`
namespace kas::z80::parser
{
    namespace x3 = boost::spirit::x3;
    
    // declare parser for Z80 register tokens
    using z80_reg_x3 = x3::rule<struct X_reg, kas::parser::kas_token>;
    BOOST_SPIRIT_DECLARE(z80_reg_x3)
}


// expose terminals to expression subsystem
namespace kas::expression::detail
{
    // override default `kas` types in `expr_types.h`
    // undefined addresses are 16-bits
    template <> struct e_data <void> { using type = std::uint8_t;   };
    template <> struct e_addr <void> { using type = std::uint16_t;  };
    template <> struct err_msg<void> { using type = z80::error_msg; };

    //template <> struct e_float<void> : meta::id<void> {};

    // types for expression variant
    template <> struct term_types_v<defn_cpu> :
        meta::list<
              z80::z80_reg_t      // z80 register
            , z80::z80_rs_ref     // z80 register set: index_reg + offset
            > {};

    // directly parsed types
    template <> struct term_parsers_v<defn_cpu> :
        meta::list<
              z80::parser::z80_reg_x3
        //       z80::tok_z80_reg 
            > {};
}


#endif
