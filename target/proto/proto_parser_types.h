#ifndef KAS_PROTO_UC_PROTO_UC_PARSER_TYPES_H
#define KAS_PROTO_UC_PROTO_UC_PARSER_TYPES_H

// Boilerplate to allow `statement parser` to accept tgt insns
//
// Each "statment" is placed in `stmt_t` type before being evaluated

// get target stmt definitions
#include "PROTO_LC_stmt.h"
#include "parser/parser_stmt.h"

// Declare reg_t, insn_t & stmt_t parsers
namespace kas::PROTO_LC::parser
{
    namespace x3 = boost::spirit::x3;
    
    // parse insn names: defined by `insn_adder`
    using PROTO_LC_insn_x3 = x3::rule<struct _, kas::parser::kas_token>;
    BOOST_SPIRIT_DECLARE(PROTO_LC_insn_x3)

    // parse statements: defined in `PROTO_LC_parser_def.h`
    using PROTO_LC_stmt_x3 = x3::rule<struct _, PROTO_LC_stmt_t *>;
    BOOST_SPIRIT_DECLARE(PROTO_LC_stmt_x3)
}


// Add stmt_t parser to `kas::parser` lists
// NB: insn_t parser used to parse statements in `PROTO_LC_parser_def.h`
namespace kas::parser::detail
{
    // statements parsed by parser
    template <> struct stmt_ops_l<defn_cpu> :
        meta::list<
              PROTO_LC::parser::PROTO_LC_stmt_x3
            > {};
}

#endif
