#ifndef KAS_Z80_Z80_STMT_H
#define KAS_Z80_Z80_STMT_H

// Boilerplate to allow `statement parser` to accept z80 insns
//
// Each "statment" is placed in `stmt_z80` structure before being evaluated
//
// Declare z80 parsed instruction type
//
// format is regular:  opcode + [args]

// get base types for `z80_stmt`
#include "z80_arg.h"
#include "z80_hw_defns.h"

#include "target/tgt_insn.h"
#include "target/tgt_stmt.h"

namespace kas::z80
{
    // forward declare `mcode` type
    struct z80_mcode;

    // declare types for parsing
    // NB: there are 19 variants of `ld`
    using z80_insn_t = tgt::tgt_insn_t<z80_mcode, hw::hw_tst, 32>;
    using z80_stmt_t = tgt::tgt_stmt<z80_insn_t, z80_arg_t>;
}


namespace kas::z80::parser
{
    namespace x3 = boost::spirit::x3;

    // declare parser for Z80 instructions
    using z80_insn_parser_type = x3::rule<struct _insn, z80_insn_t const*>;
    BOOST_SPIRIT_DECLARE(z80_insn_parser_type)

    // XXX ???
    z80_insn_parser_type const& z80_insn_parser();
}

#endif
