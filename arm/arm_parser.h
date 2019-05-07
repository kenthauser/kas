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

//x3::rule<class _,   expr_t> parse_regset = "arm_parse_regset";
#if 0

immed:      # immed
shift:        LSL #<n>              (1 << n << 31)
            | {LSR, ASR, ROR} #<n>  (1 << n << 32)
            | RRX

offset:     [Rn, offset]
pre-index:  [Rn, offset]!
post-index  [Rn], offset

NB: offset == immed8/12
            , index register <Rm>
            , shifted index register: <Rm>, LSL #<shift>

#endif

// put regset in token to get location tagging
struct token_regset : kas_token
{
    operator expr_t()
    {
        value.set_loc(*this);
        std::cout << "tagging regset: loc = " << value.get_loc() << std::endl;
        return value;
    }

    arm_rs_ref value; 
};

// allocate `regset` from first `reg`
auto regset_init = [](auto& ctx) 
        { 
            auto& regset  = arm_reg_set::add(x3::_attr(ctx));
            x3::_val(ctx) = regset.ref();
            std::cout << "created regset: ";
            x3::_val(ctx).get().print(std::cout);
            std::cout << std::endl;
        };

// add another term to regset
auto regset_add = [](char kind)
        {
            return [kind=kind](auto& ctx) 
                {
                    x3::_val(ctx).get().binop(kind, x3::_attr(ctx));
            std::cout << "regset added: type = " << kind << ": ";
            x3::_val(ctx).get().print(std::cout);
            std::cout << std::endl;
                };
        };


auto const regset_parser = rule<class _, arm_rs_ref> {} =
    arm_reg_x3()[regset_init] >>
        *(( ',' > arm_reg_x3())[regset_add('/')]
         |( '-' > arm_reg_x3())[regset_add('-')]
         );

auto const reg_offset_parser = rule<class _, arm_rs_ref> {} =
    arm_reg_x3()[regset_init] >> 
         ('+' > expr())[regset_add('+')]
        |('-' > expr())[regset_add('-')]
        ; 

//Use `token` parser to tag ref
auto parse_regset      = token<token_regset>[regset_parser];
auto parser_reg_offset = token<token_regset>[reg_offset_parser];

auto const arg_regset = parse_regset > '}' > attr(MODE_REGSET);

auto const arg_indir = (parser_reg_offset > 
                              ( "]!" > attr(MODE_PRE_INDEXED))
                            | ( ']'  > attr(MODE_REG_OFFSET)))
                      |(arm_reg_x3() > ']' > attr(MODE_POST_INDEXED))
                      ;

// slightly complicated parser to prevent parser back-tracking
auto const arg_shift = ("LSL" > ( '#' > expr() > attr(MODE_SHIFT_LSL))
                               |( arm_reg_x3() > attr(MODE_SHIFT_LSL)))
                      |("LSR" > ( '#' > expr() > attr(MODE_SHIFT_LSR))
                               |( arm_reg_x3() > attr(MODE_SHIFT_LSR)))
                      |("ASR" > ( '#' > expr() > attr(MODE_SHIFT_ASR))
                               |( arm_reg_x3() > attr(MODE_SHIFT_ASR)))
                      |("ROR" > ( '#' > expr() > attr(MODE_SHIFT_ROR))
                               |( arm_reg_x3() > attr(MODE_SHIFT_ROR)))
                      |("RRX" > attr(32)       > attr(MODE_SHIFT_RRX))
                      ;


auto const arm_parsed_arg_def =
          expr()                        > attr(MODE_DIRECT)
        | ('#' > expr()                 > attr(MODE_IMMEDIATE))
     //   | ('[' > arg_indir)
        | ('{' > arg_regset)
        | arg_shift
        ;

    // convert "parsed pair" into arg via `tgt_arg_t` ctor
    x3::rule<class _tag_arm_arg, arm_arg_t>  arm_arg        = "arm_arg";
    x3::rule<class _tag_missing, arm_arg_t>  arm_missing    = "arm_missing";
   
    auto const arm_arg_def     = arm_parsed_arg;
    auto const arm_missing_def = eps;      // need location tagging


    BOOST_SPIRIT_DEFINE(arm_parsed_arg, arm_arg, arm_missing)
    //BOOST_SPIRIT_DEFINE(parse_regset)

    // an arm instruction is "opcode" followed by comma-separated "arg_list"
    // "no arguments" -> location tagged, default contructed `arm_arg`
    
    rule<class _arm_args, std::vector<arm_arg_t>> const arm_args = "arm_args";

    auto const arm_args_def
           = arm_arg % ','              // allow comma seperated list of args
           | repeat(1)[arm_missing]     // no args: MODE_NONE, but location tagged
           ;

    BOOST_SPIRIT_DEFINE(arm_args)

    // need two rules to get tagging 
    auto const raw_arm_stmt = rule<class _, arm_stmt_t> {} = 
                        (arm_insn_x3() > arm_args)[arm_stmt_t()];

    // Parser interface
    arm_stmt_x3 arm_stmt {"arm_stmt"};
    auto const arm_stmt_def = raw_arm_stmt;

    BOOST_SPIRIT_DEFINE(arm_stmt)
    
    // tag location for each argument
    struct _tag_arm_arg  : kas::parser::annotate_on_success
        {
            using base_t = kas::parser::annotate_on_success;
            template <typename...Ts>
            void on_success(Ts&&...ts)
            {
                std::cout << "tagging" << std::endl;
                base_t::on_success(std::forward<Ts>(ts)...);
            }
        };
    struct _tag_missing  : kas::parser::annotate_on_success {};
    struct _tag_arm_stmt : kas::parser::annotate_on_success {}; 
}

#endif
