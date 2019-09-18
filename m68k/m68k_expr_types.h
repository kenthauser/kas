#ifndef KAS_M68K_M68K_EXPR_TYPES_H
#define KAS_M68K_M68K_EXPR_TYPES_H

// Define the m68k terminals used in the expression subsystem
//
// These definitions allow the `expr_t` variant to be defined,
#include "expr/expr_types.h"

// need complete register definition to instantiate expr_t
#include "m68k_reg_types.h"

// define error_message type
#include "m68k_error_messages.h"

// forward declare m68k types
namespace kas::m68k
{
    // forward declare mcode type
    struct m68k_mcode_t;

    // forward declare m68k floating point formatter
    struct m68k_format_float;

    // utility type: m68k data is 16-bit words
    using m68k_base_t = std::uint16_t;
}

namespace kas::expression
{
    // M68K defines non-standard `extended` & `packed` formats
    //template <> struct float_fmt<void> { using type = m68k::m68k_format_float; };
    template <> struct err_msg_t<void> { using type = m68k::error_msg;   };
    
    // M68K is baased on 16-bit instructions & 32-bit addresses
    using m68k_base_t = std::uint16_t;      // XXX marked for deletion (moved to m68k namespace)
    template <> struct e_data_t <void> { using type = m68k::m68k_base_t; };
    template <> struct e_addr_t <void> { using type = std::uint32_t;     };
}

// expose terminals to expression subsystem
namespace kas::expression::detail
{
    // m68k types for expression variant
    template <> struct term_types_v<defn_cpu> :
        meta::list<
              m68k::m68k_reg_t      // m68k register
            , m68k::m68k_rs_ref     // m68k register-set reference
            > {};

    // parsers for directly parsed types
    template <> struct term_parsers_v<defn_cpu> :
        meta::list<
              m68k::parser::m68k_reg_x3
            > {};
}


#endif
