#ifndef KAS_Z80_Z80_PARSER_H
#define KAS_Z80_Z80_PARSER_H

//////////////////////////////////////////////////////////////////////////
// Parse Z80 instruction using Zilog syntax
//////////////////////////////////////////////////////////////////////////

#include "z80_parser_types.h"

#include "expr/expr.h"              // expression public interface
#include "parser/annotate_on_success.hpp"

#include <boost/spirit/home/x3.hpp>
#include <boost/fusion/include/std_pair.hpp>

namespace kas::z80::parser
{
    namespace x3 = boost::spirit::x3;
    using namespace x3;
    using namespace kas::parser;

    // X3 parser rules: convert "parsed" args to location-stampped args
    
    // NB: X3 only performs type conversions via ctors with single args.
    // However, including 'boost/fusion/std_pair.hpp" allows pairs to be parsed
    // Thus, parse args as "expr / mode" pair & pass that to `z80_arg_t` ctor

    // parse args into "value, mode" pair. 
    using z80_parsed_arg_t = std::pair<expr_t, z80_arg_mode>;
    x3::rule<class z80_p_arg,   z80_parsed_arg_t>  z80_parsed_arg = "z80_parsed_arg";
    
    auto const z80_parsed_arg_def =
              '(' > expr() > ')' > attr(MODE_INDIRECT) 
            | '#' > expr() >       attr(MODE_IMMEDIATE)
            | expr()       >       attr(MODE_DIRECT)
            ;

    // convert "parsed pair" into arg via `z80_arg_t` ctor
    x3::rule<class z80_arg    , z80_arg_t>  z80_arg        = "z80_arg";
    x3::rule<class z80_missing, z80_arg_t>  z80_missing    = "z80_missing";
   
    auto const z80_arg_def     = z80_parsed_arg;
    auto const z80_missing_def = eps;      // need location tagging


    BOOST_SPIRIT_DEFINE(z80_parsed_arg, z80_arg, z80_missing)

    // an z80 instruction is "opcode" followed by comma-separated "arg_list"
    // no arguments indicated by location tagged, default contructed `z80_arg`
    
    rule<class _z80_args, std::vector<z80_arg_t>> const z80_args = "z80_args";

    auto const z80_args_def
           = z80_arg % ','              // allow comma seperated list of args
           | repeat(1)[z80_missing]     // no args: MODE_NONE, but location tagged
           ;

    BOOST_SPIRIT_DEFINE(z80_args)

    // need two rules to get tagging 
    auto const raw_z80_stmt = rule<class _, z80_stmt_t> {} = 
                        (z80_insn_x3() > z80_args)[z80_stmt_t()];

    // Parser interface
    z80_stmt_x3 z80_stmt {"z80_stmt"};
    auto const z80_stmt_def = raw_z80_stmt;

    BOOST_SPIRIT_DEFINE(z80_stmt)
    
    // tag location for each argument
    struct z80_arg       : kas::parser::annotate_on_success {};
    struct z80_missing   : kas::parser::annotate_on_success {};
    struct _tag_z80_stmt : kas::parser::annotate_on_success {}; 
}

#endif
