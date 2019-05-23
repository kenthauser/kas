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

//x3::rule<class _,   expr_t> parse_regset = "arm_parse_regset";
#if 0

ARM ARGUMENT FORMATs

direct:     label

immed:      # immed

const shift:   LSL #<n>              (1 << n << 31)
            | {LSR, ASR, ROR} #<n>  (1 << n << 32)
            | RRX

reg shift:    ASR, LSL, LSR, ROR (*not* RRX)


reg:        Rn

reg_update: Rn!


reg_offset: [Rn, offset]
pre-index:  [Rn, offset]! (ie offset with write-back)
post-index: [Rn], offset  (ie offset is next arg & is written back)

NB: offset == +/- immed
            | +/- index register <Rm>
            | shifted index register: <Rm>, LSL #<shift>

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

// slightly complicated parser to prevent back-tracking
auto const parse_shift = rule<class _, arm_shift_arg> {"parse_shift"}
        = (lit("lsl") >> ('#' > expr()  )) [arm_shift_arg(ARM_SHIFT_LSL)]
        | (lit("lsl")  >  arm_reg_x3()   ) [arm_shift_arg(ARM_SHIFT_LSL_REG)]
        | (lit("lsr") >> ('#' > expr()  )) [arm_shift_arg(ARM_SHIFT_LSR)]
        | (lit("lsr")  >  arm_reg_x3()   ) [arm_shift_arg(ARM_SHIFT_LSR_REG)]
        | (lit("asr") >> ('#' > expr()  )) [arm_shift_arg(ARM_SHIFT_ASR)]
        | (lit("asr")  >  arm_reg_x3()   ) [arm_shift_arg(ARM_SHIFT_ASR_REG)]
        | (lit("ror") >> ('#' > expr()  )) [arm_shift_arg(ARM_SHIFT_ROR)]
        | (lit("ror")  >  arm_reg_x3()   ) [arm_shift_arg(ARM_SHIFT_ROR_REG)]
        | (lit("rrx")  >  attr(expr_t()) ) [arm_shift_arg(ARM_SHIFT_RRX)]
        ;

// helpers for `parse_indir_modes`
auto get_sign = rule<class _, int> {}
                = lit('-') > attr(1)
                | lit('+') > attr(0)
                | attr(0)
                ;

auto get_write_back = rule<class _, int> {}
                = lit('!') > attr(1)
                | attr(0)
                ;

// NB: the `attr(arm_reg_t())` terms are to simplify `indir_terms::operator()`
auto const parse_indir_terms = rule<class _, arm_indirect> {"indir_terms"}
        // post-indexed
        = (']' >> (',' > (('#' > get_sign > expr())
                                            [arm_indirect(ARM_POST_INDEX_IMM)]
                          |(get_sign > arm_reg_x3() > -parse_shift)
                                            [arm_indirect(ARM_POST_INDEX_REG)]
                          )))

        // reg-update
        | (']' >> ('!' > attr(arm_indirect(ARM_INDIR_REG_WB))))

        // reg-indirect
        | (']'         > attr(arm_indirect(ARM_INDIR_REG)))

        // pre-indexed, immed
        | (',' >> ('#' > get_sign > expr() > ']' > get_write_back))
                    [arm_indirect(ARM_PRE_INDEX_IMM)]

        // pre-indexed, reg
        | (',' >> (get_sign > arm_reg_x3() > -parse_shift > ']' > get_write_back))
                    [arm_indirect(ARM_PRE_INDEX_REG)]
        ;
       
// add base register to `indir` value
auto add_base_reg = [](auto& ctx)
    {
        auto& args  = x3::_attr(ctx);
        auto& base  = boost::fusion::at_c<0>(args);
        auto& indir = boost::fusion::at_c<1>(args);
        indir.base_reg  = base;
        x3::_val(ctx) = indir;
    };

auto const parse_indir = rule<class _, arm_indirect> {"parse_indirect"}
        = (arm_reg_x3() > parse_indir_terms)[add_base_reg];
        
       

// arguments parsed as `expr/MODE` pair
using arm_parsed_arg_t = std::pair<expr_t, arm_arg_mode>;
auto const arm_parsed_expr = rule<class _, arm_parsed_arg_t> {"arm_parsed_arg"}
        = (expr() > (('!' > attr(MODE_REG_UPDATE))
                     |      attr(MODE_DIRECT)
                     ))
        | ('#' > expr()    > attr(MODE_IMMEDIATE))
        | ('{' > arg_regset)
        ;

// include more complex parsed args (1 rule for each of the 3 `arm_arg_t` ctors)
auto const raw_parsed_arg = rule<class _, arm_arg_t> { "raw_parsed_arg" }
       = arm_parsed_expr
       | '[' > parse_indir
       | parse_shift
       ;

// convert "parsed pair" into arg via `tgt_arg_t` ctor
x3::rule<class _tag_arm_arg, arm_arg_t>  arm_arg        = "arm_arg";
x3::rule<class _tag_missing, arm_arg_t>  arm_missing    = "arm_missing";

auto const arm_arg_def     = raw_parsed_arg;
auto const arm_missing_def = eps;      // need location tagging


BOOST_SPIRIT_DEFINE(arm_arg, arm_missing)
//BOOST_SPIRIT_DEFINE(parse_regset)

// an arm instruction is "opcode" followed by comma-separated "arg_list"
// "no arguments" -> location tagged, default contructed `arm_arg`
auto const arm_args = rule<class _, std::vector<arm_arg_t>> {"arm_args"}
       = arm_arg % ','              // allow comma seperated list of args
       | repeat(1)[arm_missing]     // no args: MODE_NONE, but location tagged
       ;

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
