#ifndef KAS_ARM_ARM_PARSER_H
#define KAS_ARM_ARM_PARSER_H

//////////////////////////////////////////////////////////////////////////
// Parse ARM instruction using Zilog syntax
//////////////////////////////////////////////////////////////////////////

#include "arm_parser_types.h"

#include "expr/expr.h"              // expression public interface
#include "parser/annotate_on_success.hpp"

#include <boost/spirit/home/x3.hpp>
#include <boost/fusion/include/std_pair.hpp>

namespace kas::arm::parser
{
    namespace x3 = boost::spirit::x3;
    using namespace x3;
    using namespace kas::parser;

    // X3 parser rules: convert "parsed" args to location-stampped args
    
    // NB: X3 only performs type conversions via ctors with single args.
    // However, including 'boost/fusion/std_pair.hpp" allows pairs to be parsed
    // Thus, parse args as "expr / mode" pair & pass that to `arg_t` ctor

    // parse args into "value, mode" pair. 
    using arm_parsed_arg_t = std::pair<expr_t, arm_arg_mode>;
    x3::rule<class _,   arm_parsed_arg_t>  arm_parsed_arg = "arm_parsed_arg";
    
    auto const arm_parsed_arg_def =
              '(' > expr() > ')' > attr(MODE_INDIRECT) 
            | '#' > expr() >       attr(MODE_IMMEDIATE)
            | expr()       >       attr(MODE_DIRECT)
            ;

    // convert "parsed pair" into arg via `tgt_arg_t` ctor
    x3::rule<class _tag_arm_arg, arm_arg_t>  arm_arg        = "arm_arg";
    x3::rule<class _tag_missing, arm_arg_t>  arm_missing    = "arm_missing";
   
    auto const arm_arg_def     = arm_parsed_arg;
    auto const arm_missing_def = eps;      // need location tagging


    BOOST_SPIRIT_DEFINE(arm_parsed_arg, arm_arg, arm_missing)

    // an arm instruction is "opcode" followed by comma-separated "arg_list"
    // "no arguments" -> location tagged, default contructed `arm_arg`
    
    rule<class _arm_args, std::vector<arm_arg_t>> const arm_args = "arm_args";

    auto const arm_args_def
           = arm_arg % ','              // allow comma seperated list of args
           | repeat(1)[arm_missing]     // no args: MODE_NONE, but location tagged
           ;

    BOOST_SPIRIT_DEFINE(arm_args)

    // ARM: clear the index reg prefix
    auto reset_args = [](auto& ctx) { arm_arg_t::reset(); };

    // need two rules to get tagging 
    auto const raw_arm_stmt = rule<class _, arm_stmt_t> {} = 
                        (arm_insn_x3() > eps[reset_args] > arm_args)[arm_stmt_t()];

    // Parser interface
    arm_stmt_x3 arm_stmt {"arm_stmt"};
    auto const arm_stmt_def = raw_arm_stmt;

    BOOST_SPIRIT_DEFINE(arm_stmt)
    
    // tag location for each argument
    struct _tag_arm_arg  : kas::parser::annotate_on_success {};
    struct _tag_missing  : kas::parser::annotate_on_success {};
    struct _tag_arm_stmt : kas::parser::annotate_on_success {}; 
}

#endif
