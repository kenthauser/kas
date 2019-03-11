#ifndef KAS_M68K_EXPR_TYPES_H
#define KAS_M68K_EXPR_TYPES_H

// Define the m68k terminals used in the expression subsystem
//
// These definitions allow the `expr_t` variant to be defined,

#include "expr/expr_types.h"

// need complete register definition to instantiate expr_t
#include "m68k_hw_defns.h"
#include "m68k_reg_types.h"

// define error_message type
#include "m68k_error_messages.h"

namespace kas::m68k
{
    // M68K is baased on 16-bit instructions & 32-bit addresses
    using m68k_base_t = std::uint16_t;
    template <> struct e_addr_ t<void> { using type = std::uint16_t;   };
    template <> struct err_msg_t<void> { using type = m68k::error_msg; };
}

// expose terminals to expression subsystem
namespace kas::expression::detail
{
    // types for expression variant
    template <> struct term_types_v<defn_cpu> :
        meta::list<
              m68k::m68k_reg        // m68k register
            , m68k::m68k_rs_ref     // m68k register-set reference
            > {};

    // directly parsed types
    template <> struct term_parsers_v<defn_cpu> :
        meta::list<
              m68k::parser::m68k_reg_x3
            > {};
}


#endif
