#ifndef KAS_M68K_TYPES_H
#define KAS_M68K_TYPES_H

// Define the m68k terminals used in the expression subsystem
//
// These definitions allow the `expr_t` variant to be defined,

#include "expr/expr_types.h"

// need complete register definition to instantiate expr_t
#include "m68k_hw_defns.h"
#include "m68k_reg_types.h"

// define error_message type
#include "m68k_error_messages.h"

#include "target/tgt_insn.h"

namespace kas::m68k
{
// M68K is baased on 16-bit instructions & 32-bit addresses
using m68k_base_t = std::uint16_t;
using m68k_addr_t = std::uint32_t;

namespace opc
{
    // declare opcode groups (ie: include files)
    using m68k_insn_defn_groups = meta::list<
          struct OP_M68K_GEN
        , struct OP_M68K_020
        , struct OP_M68K_040
        , struct OP_M68K_060
        , struct OP_M68K_CPU32
        , struct OP_M68K_68881
        , struct OP_M68K_68551
        , struct OP_COLDFIRE
        >;

    template <typename=void> struct m68k_insn_defn_list : meta::list<> {};

    // declare M68K INSN: 6 args, 32 opcodes with same name
    using m68k_insn_t = tgt::tgt_insn_t<struct m68k_opcode_t, 6, 32>;
}

using opc::m68k_insn_t;
using m68k_insn_ref = std::reference_wrapper<m68k_insn_t const>;

// declare parsed argument
struct m68k_arg_t;
}

// expose terminals to expression subsystem
namespace kas::expression::detail
{
    // types for expression variant
    template <> struct term_types_v<defn_cpu> :
        meta::list<
              m68k::m68k_reg        // m68k register
            , m68k::m68k_rs_ref     // m68k register-set reference
            , m68k::m68k_insn_ref   // m68k insn reference
            > {};

    // directly parsed types
    template <> struct term_parsers_v<defn_cpu> :
        meta::list<
              m68k::parser::X_m68k_reg_parser_p
            > {};
}


#endif
