#ifndef KAS_Z80_TYPES_H
#define KAS_Z80_TYPES_H

// Define the z80 terminals used in the expression subsystem
//
// These definitions allow the `expr_t` variant to be defined,

#include "expr/expr_types.h"

// need complete register definition to instantiate expr_t
#include "z80_hw_defns.h"
#include "z80_reg_types.h"

// define error_message type
#include "z80_error_messages.h"

#include "target/tgt_insn.h"

namespace kas::z80
{
// Z80 is baased on 16-bit instructions & 32-bit addresses
using z80_base_t = std::uint16_t;
using z80_addr_t = std::uint32_t;

namespace opc
{
    // declare opcode groups (ie: include files)
    using z80_insn_defn_groups = meta::list<
          struct OP_Z80_GEN
        >;

    template <typename=void> struct z80_insn_defn_list : meta::list<> {};

    // declare Z80 INSN: 2 args, 16 opcodes with same name
    using z80_insn_t = tgt::tgt_insn_t<struct z80_opcode_t, 2, 16>;
}

using opc::z80_insn_t;
using z80_insn_ref = std::reference_wrapper<z80_insn_t const>;

// declare parsed argument
struct z80_arg_t;
}

// expose terminals to expression subsystem
namespace kas::expression::detail
{
    // types for expression variant
    template <> struct term_types_v<defn_cpu> :
        meta::list<
              z80::z80_reg_t      // z80 register
            , z80::z80_insn_ref   // z80 insn reference
            > {};

    // directly parsed types
    template <> struct term_parsers_v<defn_cpu> :
        meta::list<
              z80::parser::z80_reg_parser_p
            > {};
}


#endif
