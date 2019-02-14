#ifndef KAS_Z80_Z80_PARSER_DEFS_H
#define KAS_Z80_Z80_PARSER_DEFS_H

// Boilerplate to allow `statement parser` to accept tgt insns
//
// Each "statment" is placed in `stmt_t` type before being evaluated

// get target stmt definitions
#include "z80_stmt.h"
#include "parser/parser_stmt.h"
// XXX need x3 include...

// Declare reg_t, insn_t & stmt_t parsers
namespace kas::z80::parser
{
    namespace x3 = boost::spirit::x3;

    // parse insn names: defined by `insn_adder`
    using z80_insn_x3 = x3::rule<struct _insn, z80_insn_t const *>;
    BOOST_SPIRIT_DECLARE(z80_insn_x3)

    // parse statements: defined in `z80_parser_def.h`
    using z80_stmt_x3 = x3::rule<struct _stmt, z80_stmt_t>;
    BOOST_SPIRIT_DECLARE(z80_stmt_x3)
}


// Add stmt_t parser to `kas::parser` lists
// NB: insn_t parser used to parse statements in `z80_parser_def.h`
namespace kas::parser::detail
{
    // target types for stmt variant
    template <> struct parser_type_l<defn_cpu> :
        meta::list<
              z80::z80_stmt_t
            > {};

    // statements parsed by parser
    template <> struct parser_stmt_l<defn_cpu> :
        meta::list<
              z80::parser::z80_stmt_x3
            > {};

    // NB: no z80-specific label parsers
}

#endif
