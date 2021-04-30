#ifndef KAS_PROTO_UC_PROTO_UC_PARSER_H
#define KAS_PROTO_UC_PROTO_UC_PARSER_H

//////////////////////////////////////////////////////////////////////////
// Parse PROTO_UC instruction
//////////////////////////////////////////////////////////////////////////

#include "parser/parser.h"
#include "parser/token_parser.h"

#include <boost/fusion/include/std_pair.hpp>

namespace kas::PROTO_LC::parser
{
// expose `missing` parser
using tok_missing = expression::tok_missing;

// Define PROTO_UC Instruction Argument parser.

// NB: X3 only performs type conversions via ctors with single args.
// However, including "boost/fusion/std_pair.hpp" allows pairs to be parsed.
// Thus, parse args as 'token / mode' pair & pass that to `arg_t` ctor

// EXAMPLE: use Z80 zilog syntax...
using PROTO_LC_parsed_arg_t = std::pair<kas_token, PROTO_LC_arg_mode>;
auto const PROTO_LC_arg = x3::rule<class _, PROTO_LC_parsed_arg_t> {"PROTO_LC_arg"}
        = '(' > expr() > ')' > x3::attr(MODE_INDIRECT) 
        | '#' > expr() >       x3::attr(MODE_IMMEDIATE)
        | expr()       >       x3::attr(MODE_DIRECT)
        ;

// parse comma separated arg-list (NB: use single `tok_missing` for empty list)
auto const PROTO_LC_args = x3::rule<class _, std::vector<PROTO_LC_arg_t>> {"PROTO_LC_args"}
       = PROTO_LC_arg % ','              // allow comma seperated list of args
       | x3::repeat(1)[tok_missing()] // no args: MODE_NONE, but location tagged
       ;

//
// Define actual PROTO_UC Instruction parser. 
//

// Define statement rule:
//  EXAMPLE: `PROTO_LC_insn` with vector of `PROTO_LC_arg_t`s -> `PROTO_LC_stmt_t`
auto const PROTO_LC_stmt_def = (PROTO_LC_insn_x3() > PROTO_LC_args)[PROTO_LC_stmt_t()];

// c++ magic for external linkage
PROTO_LC_stmt_x3 PROTO_LC_stmt {"PROTO_LC_stmt"};
BOOST_SPIRIT_DEFINE(PROTO_LC_stmt)
}

#endif
