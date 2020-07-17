#ifndef KAS_Z80_Z80_PARSER_H
#define KAS_Z80_Z80_PARSER_H

//////////////////////////////////////////////////////////////////////////
// Parse Z80 instruction using Zilog syntax
//////////////////////////////////////////////////////////////////////////

#include "parser/parser.h"
#include "parser/token_parser.h"

#include <boost/fusion/include/std_pair.hpp>

namespace kas::z80::parser
{
// expose `missing` parser
using tok_missing = expression::tok_missing;

// Define Z80 Instruction Argument parser.

// NB: X3 only performs type conversions via ctors with single args.
// However, including "boost/fusion/std_pair.hpp" allows pairs to be parsed.
// Thus, parse args as 'token / mode' pair & pass that to `arg_t` ctor

using z80_parsed_arg_t = std::pair<kas_token, z80_arg_mode>;
auto const z80_arg = x3::rule<class _, z80_parsed_arg_t> {"z80_arg"}
        = '(' > expr() > ')' > x3::attr(MODE_INDIRECT) 
        | '#' > expr() >       x3::attr(MODE_IMMEDIATE)
        | expr()       >       x3::attr(MODE_DIRECT)
        ;

// parse comma separated arg-list (NB: use single `tok_missing` for empty list)
auto const z80_args = x3::rule<class _, std::vector<z80_arg_t>> {"z80_args"}
       = z80_arg % ','              // allow comma seperated list of args
       | x3::repeat(1)[tok_missing()] // no args: MODE_NONE, but location tagged
       ;

// Define Z80 Instruction parser. 
// Declared in `z80_parser.h`. Instantiated in `z80.cc`

// NB: Z80 only allows a single index register to be parsed per instruction.
// Enforce by storing index `prefix` as static. Invoke `reset` static method 
// before parsing args.
auto reset_args = [](auto& ctx) { z80_arg_t::reset(); };

// Define statement rule (ie: `z80_insn` with vector of `z80_arg_t`s -> `z80_stmt_t`)
auto const z80_stmt_def = (z80_insn_x3() > x3::eps[reset_args] > z80_args)[z80_stmt_t()];

// c++ magic for external linkage
z80_stmt_x3 z80_stmt {"z80_stmt"};
BOOST_SPIRIT_DEFINE(z80_stmt)
}

#endif
