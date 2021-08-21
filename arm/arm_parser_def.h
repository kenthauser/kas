#ifndef KAS_ARM_ARM_PARSER_H
#define KAS_ARM_ARM_PARSER_H

//////////////////////////////////////////////////////////////////////////
// Parse ARM instruction
//////////////////////////////////////////////////////////////////////////

#include "arm.h"
#include "arm_stmt.h"
#include "arm_stmt_flags.h"
#include "arm_directives.h"
#include "arm_parser_support.h"

#include "expr/operators.h"     // for regset
#include "parser/parser.h"
#include "parser/token_parser.h"
#include "parser/annotate_on_success.hpp"

#include <boost/fusion/include/std_pair.hpp>

namespace kas::arm::parser
{
using namespace x3;
using namespace kas::parser;

//
// Parse `shift` arg
//

// NB: RRX is slightly different & uses a derived class to generate `shift_arg`
auto const parse_shift = rule<class _, arm_shift_arg> {"parse_shift"}
        = (no_case["lsl"] > -char_('#') > expr())[arm_shift_arg(ARM_SHIFT_LSL)]
        | (no_case["lsr"] > -char_('#') > expr())[arm_shift_arg(ARM_SHIFT_LSR)]
        | (no_case["asr"] > -char_('#') > expr())[arm_shift_arg(ARM_SHIFT_ASR)]
        | (no_case["ror"] > -char_('#') > expr())[arm_shift_arg(ARM_SHIFT_ROR)]
        | (no_case["rrx"])[arm_shift_arg_rrx()]
        ;

//
// Parse `indirect` arg
//

// helpers for `parse_indir_terms`
// get_sign: -{+/-} -> '+' or (omitted) => 0, '-' => 1
auto is_minus = rule<class _, int> {}
                = lit('-') > attr(1)
                | lit('+') > attr(0)
                | attr(0)
                ;
// get_write_back: {!} -> (omitted) => 0, '!' => 1
auto get_wb = rule<class _, int> {}
                = lit('!') > attr(1)
                | attr(0)
                ;
auto const parse_indir_terms = rule<class _, arm_indirect_arg> {"indir_terms"}
        // offset or pre-indexed: immed, register, or register shift
        // Modes: 2.1, 2.2, 2.3, 2.4, 2.5, 2.6
        = (',' >> (-char_('#') > is_minus > expr() > -(',' > parse_shift) >> ']'
                 > get_wb))[arm_indirect_pre_index()]
        
        // post-indexed offsets
        // Modes: 2.7, 2.8, 2.9
        | (']' >> (',' > -char_('#') > is_minus > expr() > -(',' > parse_shift)))
                [arm_indirect_post_index()]

        // reg-indirect & reg-indirect-update
        // Modes: 2.1 (zero offset), 2.4 (zero offset) 
        | (']' > get_wb)[arm_indirect_arg()]

        ;
       
// add base register to `indir` value
auto add_base_reg = [](auto& ctx)
    {
        auto& args  = x3::_attr(ctx);
        auto& base  = boost::fusion::at_c<0>(args);
        auto& indir = boost::fusion::at_c<1>(args);
        indir.set_base(base);       // validates base register
        x3::_val(ctx)  = indir;     // return validated `indir` type
    };
auto const parse_indir = rule<class _, arm_arg_t> {"parse_indirect"}
        = (expr() > parse_indir_terms)[add_base_reg];
        
//
// Parse `register-set arg`
//
#if 0
// allocate `regset` from first `reg`
auto regset_init = [](auto& ctx) 
        {
            using tok_reg = typename arm_reg_t::token_t;
            auto& tok   = x3::_attr(ctx);
            auto  reg_p = tok_reg(tok)();
            auto& regset  = arm_reg_set_t::add(*reg_p);
            x3::_val(ctx) = regset.ref();
        };

// add another term to regset
auto regset_add = [](char op)
        {
            return [op=op](auto& ctx) 
                {
                    using tok_reg = typename arm_reg_t::token_t;
                    auto& tok   = x3::_attr(ctx);
                    auto  reg_p = tok_reg(tok)();
                    x3::_val(ctx).get().binop(op, *reg_p);
                };
        };

// must use `ref` because `arm_regset` is KAS_OBJECT & can't be copied nor moved
auto const parse_regset = rule<class _, arm_rs_ref> {} =
        arm_reg_x3()[regset_init] >> (',' > arm_reg_x3())[regset_add('/')]
                                    |('-' > arm_reg_x3())[regset_add('-')]
        ; 

auto const arg_regset = parse_regset > '}' > attr(MODE_REGSET);
#else

using tok_reg    = typename arm_arg_t::reg_t   ::token_t;
using tok_regset = typename arm_arg_t::regset_t::token_t;

using arm_parsed_arg_t = std::pair<kas_token, arm_arg_mode>;

auto const regset_terms = rule<class _, std::pair<char, kas_token>>{"regset term"}
        = char_("-,") > arm_reg_x3();

auto const parse_regset = rule<class _, kas_token> {"parse regset"}
       = (arm_reg_x3() > *regset_terms)[gen_regset()];

auto const arg_regset = rule<class _, arm_parsed_arg_t> {"regset"}
       = (parse_regset > '}' > ( ('^' > attr(MODE_REGSET_USER))
                              |        attr(MODE_REGSET)))
         | (ushort_     > '}' > attr(MODE_CP_OPTION))
         ;

#endif
//
// Parse simple arguments into `expr/MODE` pair
//
// Direct, Immediate, Register-set
//

auto const simple_parsed_arg = rule<class _, arm_parsed_arg_t> {"arm_parsed_arg"}
        = (expr() > (('!' > attr(MODE_REG_UPDATE))
                     |      attr(MODE_DIRECT)
                     ))
        | (lit('#') >
          ( (":lower16:"    > expr() > attr(MODE_IMMED_LOWER))
          | (":upper16:"    > expr() > attr(MODE_IMMED_UPPER))
          | (":lower0_7:"   > expr() > attr(MODE_IMMED_BYTE_0))
          | (":lower8_15:"  > expr() > attr(MODE_IMMED_BYTE_1))
          | (":upper0_7:"   > expr() > attr(MODE_IMMED_BYTE_2))
          | (":upper8_15:"  > expr() > attr(MODE_IMMED_BYTE_3))
          | (                 expr() > (('!' > attr(MODE_IMMED_UPDATE)
                                        |      attr(MODE_IMMEDIATE))
                                        ))
          ))
        | ('{' > arg_regset)
        ;

//
// include more complex parsed args (and annotate if untagged)
//
struct _tag_arg : annotate_on_success {};
auto const arm_arg = rule<_tag_arg, arm_arg_t> { "arm_arg" }
       = '[' > parse_indir
       | parse_shift
       | simple_parsed_arg
       ;

// expose `missing` parser
using tok_missing = expression::tok_missing;

// parse comma separated arg-list (NB: use single `tok_missing` for empty list)
auto const arm_args = x3::rule<class _, std::vector<arm_arg_t>> {"arm_args"}
       = arm_arg % ','              // allow comma seperated list of args
       | x3::repeat(1)[tok_missing()] // no args: MODE_NONE, but location tagged
       ;

//
// Define actual ARM Instruction parser. 
//

// ARM encodes several "options" in insn name. Decode them.
// invalid options error out in `arm_insn_t::validate_args`
auto const parse_insn = rule<class _, std::tuple<kas_token
                                               , arm_stmt_info_t>> {} =
            lexeme[(arm_insn_x3()       // insn_base_name
                >> -arm_ccode::x3()     // optional condition-code
                >> -arm_suffix::x3()    // optional suffix (after ccode)
                >> -char_("sS")         // s-flag (update flags)
                >> -(lit('.') >> char_("nNwW"))
                >> !graph               // no trailing characters
                )[gen_stmt]
                ];

// Define statement rule:
auto const arm_stmt_def = (parse_insn > arm_args)[arm_stmt_t()];

//
// ARM Directives
//

// comma separated tokens. Identifiers can have additional characters
auto const arm_ident = token<tok_arm_ident>[char_("a-zA-Z_.0-9-")];
auto const arm_dir_arg = rule<struct _, kas_token> {"arm directive arg"}
    = arm_ident | expr();

auto const arm_dir_args = rule<class _, std::vector<kas_token>> {}
    = (arm_dir_arg % ',') % ','
    | repeat(1)[expression::tok_missing()];

auto const arm_dir_def = (arm_dir_op_x3() > arm_dir_args)[arm_directive_t()];

//
// c++ magic for external linkage
//

arm_stmt_x3 arm_stmt {"arm_stmt"};
arm_dir_x3  arm_dir  {"arm directive"};
BOOST_SPIRIT_DEFINE(arm_stmt, arm_dir)
}

#endif
