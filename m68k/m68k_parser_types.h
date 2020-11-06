#ifndef KAS_M68K_M68K_PARSER_TYPES_H
#define KAS_M68K_M68K_PARSER_TYPES_H

// Boilerplate to allow `statement parser` to accept tgt insns
//
// Each "statment" is placed in `stmt_t` type before being evaluated

// get target stmt definitions
#include "m68k_stmt.h"
#include "parser/parser_stmt.h"

// Declare reg_t, insn_t & stmt_t parsers
namespace kas::m68k::parser
{
    namespace x3 = boost::spirit::x3;
    
    // parse insn names: defined by `insn_adder`
    using m68k_insn_x3 = x3::rule<struct _, kas::parser::kas_token>;
    BOOST_SPIRIT_DECLARE(m68k_insn_x3)

    // parse statements: defined in `m68k_parser_def.h`
    // XXX for reasons I don't understand, `x3/nonterminal/detail/rule.hpp:331`
    // XXX determines that `m68k_stmt_x3` should be evaluated with attribute
    // XXX type of `x3::unused_type`. Thus, need to parse with internal rule for
    // XXX `m68k_stmt_t`, and then have second `rule` `tag` it. 
    //using m68k_stmt_x3 = x3::rule<kas::parser::annotate_on_success, m68k_stmt_t>;
    using m68k_stmt_x3 = x3::rule<struct _tag_m68k_stmt, m68k_stmt_t>;
    BOOST_SPIRIT_DECLARE(m68k_stmt_x3)
}


// Add stmt_t parser to `kas::parser` lists
// NB: insn_t parser used to parse statements in `m68k_parser_def.h`
namespace kas::parser::detail
{
    // target types for stmt variant
    template <> struct parser_type_l<defn_cpu> :
        meta::list<
              m68k::m68k_stmt_t
            > {};

    // statements parsed by parser
    // XXX
    //template <> struct parser_stmt_l<defn_cpu> :
    template <> struct stmt_ops_l<defn_cpu> :
        meta::list<
              m68k::parser::m68k_stmt_x3
            > {};

    // NB: no m68k-specific label parsers
}

#endif
