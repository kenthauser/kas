#ifndef KAS_M68K_M68K_PARSER_H
#define KAS_M68K_M68K_PARSER_H

//////////////////////////////////////////////////////////////////////////
// Parse M68K instruction
//////////////////////////////////////////////////////////////////////////


// Complex instruction set processors are called "complex" for a reason.
//
// The MIT m68k indirect format allows great flexibility in specifying
// the operands. This code normalizes the various ways of specifying
// arguments into the `m68k_parsed_arg_t` type. The code in module
// `m68k::m68k_parser_support.h` analyzes the various components, processor
// addressing modes, and run-time configuration to generate a
// `m68k_arg_t` which is used by all other code.
//
// The MIT direct format uses '#', '@', '@+', '@-' prefixes/postfixes
// to inform interpretation of the single argument. The class
// `m68k_parsed_arg_t` processes the single argument (plus pre/postfix)
// arguments.
//
// The indirect formats are generally used to specify register indirect
// displacement and register indirect index operations. These are quite
// complex in their general form.
//
// 1. The indirect format allows an optionally suppressed base register
// (normally an address register or PC, but also on an 020+, a data register)
// to be named.
//
// 2. In addition to a base register, one or two additional "indirection"
// operations may be specified.
//
// 3. Each of the "indirection" operations may specify an optionally
// sized (word/long) and scaled (x1,2,4,8) "general register" (aka the
// index register) and a displacement. The displacement and register
// may be in either order, but each may only be specified once.
//
// 4. The "index" register may be named in either the first or second
// indirect specification, but not both. If a data register is used for
// the base register, it is implicitly added to the first index specification
// and the base register is suppressed.
//
// 5. Long multiply & divides can involve 64-bit operands/results which
// require two data registers: two registers separated by a `:`
//
// 6. Bit Field operators require an bit-offset and bitfield-width
// Offset & width can be constants (0-31) or data registers.
//
// Other complexities apply. Implementation issues include offset values
// which become zero (or go out of range) during evaluation. Not all formats
// are supported by all processors in the family.
//
// In other words...complex.

#ifdef BOOST_SPIRIT_X3_DEBUG
#include <iostream>
#endif

#include "m68k.h"
#include "m68k_stmt.h"
#include "m68k_parser_support.h"    // evaluate complex m68k args
#include "m68k_stmt_flags.h"

#include "expr/expr.h"              // expression public interface
#include "parser/token_parser.h"
#include "parser/annotate_on_success.hpp"

#include <boost/spirit/home/x3.hpp>


namespace kas::m68k::parser
{
    namespace x3 = boost::spirit::x3;
    using namespace x3;
    using namespace kas::parser;

    //////////////////////////////////////////////////////////////////////////
    //  M68K "tok_sized_fixed" fixed argument parser
    //
    // required because `751.w` parses as floating point
    //////////////////////////////////////////////////////////////////////////
    
    // declare token & parser
    using tok_sized_fixed = kas::parser::token_defn_t<KAS_STRING("MOTO_SIZED_EXPR")
                                                    , e_fixed_t>;
   
    // get parser for "standard" fixed parser. Add suffix for MOTO sized variant
    using fixed_p = typename expression::detail::tok_fixed::parser_t;

    auto const sized_fixed_p = token<tok_sized_fixed>
                            [fixed_p() >> lit('.') >> omit[char_("wWlL")]];

    // complete declared parser for instantiation
    m68k_sized_fixed_x3 moto_sized_fixed { "moto_sized_fixed" };
    auto const moto_sized_fixed_def = sized_fixed_p;
    BOOST_SPIRIT_DEFINE(moto_sized_fixed)
    
    //////////////////////////////////////////////////////////////////////////
    // Parse M68K argument using MIT syntax
    //////////////////////////////////////////////////////////////////////////

   // an m68k argument is "expression" augmented by "address mode"
    auto m68k_disp = [](auto&& mode) { return attr(m68k_displacement{mode}); };
    auto z_expr    = [] { return attr(m68k_arg_term_t{0, 0, P_SCALE_Z}); };

    // NB: the `no_skip` directives prevent matching whitespace following args
    auto const mit_size =
              no_case[":w"] > attr(M_SIZE_WORD)
            | no_case[":l"] > attr(M_SIZE_LONG)
            | no_skip[eps]  > attr(M_SIZE_AUTO)
            ;
    auto const mit_scale =
              ":1"         > attr(P_SCALE_1)
            | ":2"         > attr(P_SCALE_2)
            | ":4"         > attr(P_SCALE_4)
            | ":8"         > attr(P_SCALE_8)
            | no_skip[eps] > attr(P_SCALE_AUTO)
            ;

    // parse m68k displacement element
    auto const moto_size =
              no_case[".w"] > attr(M_SIZE_WORD)
            | no_case[".l"] > attr(M_SIZE_LONG)
            | no_skip[eps]  > attr(M_SIZE_AUTO)
            ;
    auto const moto_scale =
              "*1"          > attr(P_SCALE_1)
            | "*2"          > attr(P_SCALE_2)
            | "*4"          > attr(P_SCALE_4)
            | "*8"          > attr(P_SCALE_8)
            | no_skip[eps]  > attr(P_SCALE_AUTO)
            ;
    using tok_invalid_term  = token_defn_t<KAS_STRING("INVALID_TERM")>;
    auto const invalid_term = token<tok_invalid_term>[omit[+graph - ",)"]];


    x3::rule<class _mit_s_s , m68k_arg_term_t> const mit_ss_p  = "mit_ss_p";
    x3::rule<class _moto_s_s, m68k_arg_term_t> const moto_ss_p = "moto_size_scale";
    x3::rule<class _err_s_s , m68k_arg_term_t> const err_ss_p  = "m68k_term_error";
    auto const moto_ss_p_def = expr()         > moto_size         > moto_scale;
    auto const mit_ss_p_def  = expr()         > mit_size          > mit_scale;
    auto const err_ss_p_def  = invalid_term   > mit_size          > mit_scale;

    // auto const m68k_ss_p = expr() > m68k_size > m68k_scale;

    BOOST_SPIRIT_DEFINE(moto_ss_p, mit_ss_p, err_ss_p)

    // parse optional list of "indirect" terms. eg: "@(d1:4, 12)"
    // allow "@()" to parse & handle semantics in "m68k_parser_support.h"
    // NB: require "x3::rule" because of `expect` ')'
#if 0
    auto const mit_indirect_arg = lit('@') >> ('(' > (mit_ss_p % ',') > ')');
#else
    struct _mit_indirect_arg : annotate_on_success{};
    auto const mit_indirect_arg = 
        rule<_mit_indirect_arg, std::vector<m68k_arg_term_t>> {"mit_indirect_arg"}\
                    = (lit('@') >> lit('(')) >      // prefix parsed...
                        ( ((mit_ss_p % ',') > ')')
                        | (lit(')') > repeat(1)[z_expr()])
                        | ((err_ss_p % ',') > lit(')')));
#endif
    struct moto_indir
    {
        // initialize m68k_parser_support.h:m68k_displacement from parsed data
        template <typename Context>
        void operator()(Context const& ctx) const
        {
            // get list of arguments
            auto& args  = x3::_attr(ctx);
            auto& ess_v = boost::fusion::at_c<0>(args);
            auto& mode  = boost::fusion::at_c<1>(args);

            m68k_displacement disp{mode};

#ifdef TRACE_M68K_PARSE
            std::cout << "moto_indir: parsed_mode = " << mode << ": ";
            for (auto& arg : ess_v)
            {
                std::cout << arg;
            }
            std::cout << std::endl;
#endif
            // get target list
            auto& out_list_list = disp.args;
            out_list_list.emplace_back();       // XXX create empty slot for base reg
            auto& out_list = out_list_list.back();

            // split into inner/outer as required
            // scan for P_SCALE_Z to start next...

            for (auto& arg : ess_v)
            {
                if (arg.scale == P_SCALE_Z)
                {
                    out_list_list.emplace_back();
                    out_list = out_list_list.back();
                }
                out_list.push_back(std::move(arg));
            }

            x3::_val(ctx) = disp;
        }
    };

    auto const moto_indir_suffix = rule<struct _sfx_tag, int> {} =
              ( ")+&"          > attr(PARSE_INCR | PARSE_MASK))
            | ( ")+"           > attr(PARSE_INCR))
            | ( no_case[").w"] > attr(PARSE_INDIR_W))
            | ( no_case[").l"] > attr(PARSE_INDIR_L))
            | ( ")&"           > attr(PARSE_INDIR | PARSE_MASK))
            | ( ")"            > attr(PARSE_INDIR))
            ;

    struct moto_outer_fn
    {
        template <typename Context>
        void operator()(Context const& ctx) const
        {
            auto& args  = x3::_attr(ctx);
            auto& inner = boost::fusion::at_c<0>(args);
            auto& ess_v = boost::fusion::at_c<1>(args);

            auto& out_list_list = inner.args;
            out_list_list.emplace_back();
            auto& out_list = out_list_list.back();

            for (auto& arg : ess_v)
                out_list.push_back(std::move(arg));

            x3::_val(ctx) = inner;
        }
    };

    x3::rule<class _moto_ind, m68k_displacement>  const moto_indirect = "moto_indirect";
    x3::rule<class _inner_tag,  m68k_displacement> moto_inner = "moto_inner";
    x3::rule<class _outer_tag,  m68k_displacement> moto_outer = "moto_outer";
    x3::rule<class _out_in_tag, m68k_displacement> moto_outer_inner = "moto_outer_inner";

    auto const moto_outer_inner_def =
            (lit('[') > (moto_ss_p % ',') > ']' > attr(PARSE_INDIR))[moto_indir()];

    auto const moto_outer_def =
            (moto_outer_inner > (moto_ss_p % ',') > ')')[moto_outer_fn()];

    auto const moto_inner_def =
            ((moto_ss_p % ',') > moto_indir_suffix)[moto_indir()];

    auto const moto_indirect_def = moto_inner;//moto_outer | moto_inner;

    BOOST_SPIRIT_DEFINE(moto_inner, moto_outer, moto_outer_inner)
    BOOST_SPIRIT_DEFINE(moto_indirect)

    x3::rule<class _attr,     m68k_displacement> const mit_arg_suffix  = "mit_arg_suffix";
    x3::rule<class _mit_pair, m68k_displacement> const mit_pair = "mit_pair";
    auto const mit_pair_def = (+mit_ss_p > attr(PARSE_PAIR))[moto_indir()];
    BOOST_SPIRIT_DEFINE(mit_pair)

    // the trailing ampersand formats are for coldfire MAC MASK format
    // don't know if MIT formats ever used on coldfire MAC, but parse anyway.
    auto const mit_arg_suffix_def =
              attr(PARSE_INDIR)  >> +mit_indirect_arg
            | ('('          > moto_indirect)
            | "@+&"         > m68k_disp(PARSE_INCR  | PARSE_MASK)
            | "@+"          > m68k_disp(PARSE_INCR)
            | "@-&"         > m68k_disp(PARSE_DECR  | PARSE_MASK)
            | "@-"          > m68k_disp(PARSE_DECR)
            | "@&"          > m68k_disp(PARSE_INDIR | PARSE_MASK)
            | "@"           > m68k_disp(PARSE_INDIR)
            | ':'           > mit_pair
            // motorola allows ".w" & ".l" suffixes on constants
            // coldfire allows ".u" & ".l" suffixes on registers for MAC
            | no_case[".w"] > m68k_disp(PARSE_SFX_W)
            | no_case[".l"] > m68k_disp(PARSE_SFX_L)
            | no_case[".u"] > m68k_disp(PARSE_SFX_U)
            | /* default */   m68k_disp(PARSE_DIR)
            ;

    auto const m68k_parsed_arg_def =
               '('  > z_expr()      > moto_indirect
            |  "-(" > moto_ss_p > 
                            (  ")&" > m68k_disp(PARSE_DECR | PARSE_MASK)
                             | ")"  > m68k_disp(PARSE_DECR)
                            )
            | '#' > mit_ss_p        > m68k_disp(PARSE_IMMED)
            |  mit_ss_p             > mit_arg_suffix
            ;

    // parse bitfield
    x3::rule<class _mit_bf_p, m68k_displacement> const mit_bitfield     = "mit_bitfield";
    x3::rule<class _bf_arg_tag, m68k_arg_term_t> const mit_bitfield_arg = "mit_bitfield_arg";

    auto const mit_bitfield_arg_def = -lit('#') > mit_ss_p;
    auto const mit_bitfield_def = ((mit_bitfield_arg % ',') > '}' > attr(PARSE_BITFIELD))[moto_indir()];
    auto const parsed_bf_arg_def = '{' > z_expr() > mit_bitfield;

    //
    // X3 parser rules: First rules to "parse" args, bitfields, and "missing"
    //
    
    x3::rule<class _m68k_arg, m68k_parsed_arg_t> const m68k_parsed_arg = "m68k_parsed_arg";
    x3::rule<class _bf_arg,   m68k_parsed_arg_t> const parsed_bf_arg   = "parsed_bf_arg";
    
    struct _m68k_arg     : kas::parser::annotate_on_success {};
    
    BOOST_SPIRIT_DEFINE(mit_arg_suffix)
    BOOST_SPIRIT_DEFINE(mit_bitfield, mit_bitfield_arg)
    BOOST_SPIRIT_DEFINE(m68k_parsed_arg, parsed_bf_arg)

    //
    // X3 parser rules: convert "parsed" args to location-stampped args
    //
    
    x3::rule<class m68k_arg,     m68k_arg_t>  m68k_arg     = "m68k_arg";
    x3::rule<class m68k_bf_arg,  m68k_arg_t>  m68k_bf_arg  = "m68k_bf_arg";
    x3::rule<class m68k_missing, m68k_arg_t>  m68k_missing = "m68k_missing";

    auto const m68k_arg_def     = m68k_parsed_arg;
    auto const m68k_bf_arg_def  = parsed_bf_arg;
    auto const m68k_missing_def = eps;      // need location tagging


    // tag location for each argument
    struct m68k_arg     : annotate_on_success {};
    struct m68k_bf_arg  : annotate_on_success {};
    struct m68k_missing : annotate_on_success {};

    BOOST_SPIRIT_DEFINE(m68k_arg, m68k_bf_arg, m68k_missing)

    // Parse comma-separated list (or empty list)
    rule<class _m68k_args, std::vector<m68k_arg_t>> const m68k_args = "m68k_args";
    auto const m68k_args_def
           = (m68k_arg >> *((',' > m68k_arg) | m68k_bf_arg))
           | repeat(1)[m68k_missing]        // MODE_NONE, but location tagged
           ;

    BOOST_SPIRIT_DEFINE(m68k_args)


// an m68k instruction is "opcode" followed by comma-separated "arg_list".
// `bitfield` breaks regularity. Allow bitfield to follow arg w/o comma
// no arguments indicated by location tagged `m68k_arg` with type MODE_NONE

// set "m68k_stmt_info_t" info based on condition codes & other insn-name flags
auto gen_stmt = [](auto& ctx)
    {
        // result is `stmt_t`
        m68k_stmt_info_t info;
        
        // unpack args & evaluate
        auto& parts    = x3::_attr(ctx);
        auto& insn_tok = boost::fusion::at_c<0>(parts);
        auto& ccode    = boost::fusion::at_c<1>(parts);
        auto& width    = boost::fusion::at_c<2>(parts);

        // all (and only) floating point insns begin with `f`
        // NB: less obvious than `std::tolower()`, but faster for specific case
        //info.is_fp = (insn_tok.begin()[0] | ('f' - 'F')) == 'f';

        if (ccode)
        {
            // need to determine which `ccode` to extract
            auto& insn     = *m68k_stmt_t::insn_tok_t(insn_tok)();
            auto  mcode_p  = insn.get_first_mcode_p();
            if (mcode_p)
            {
                info.cpid     = mcode_p->get_cpid();
                std::cout << "parser::gen_stmt: cpid = " << std::hex << info.cpid << std::endl;
            }
     
            // different condition code maps for general & fp insns
            info.has_ccode = true;
            //info.fp_ccode  = info.is_fp;
            auto code = m68k_ccode::code(*ccode, info.cpid);

            std::cout << "parser:get_ccode = " << +code << std::endl;

            if (code < 0)
            {
                // invalid condition code
                x3::_pass(ctx) = false;
                return;
            }
            else
                info.ccode = code;
        }
        
        // if `width` suffix specified, process it
        if (width)
        {
            auto cp = width->begin();
            if (*cp == '.')
                ++cp;           // consume dot
            switch (*cp)
            {
                case 'l': case 'L': info.arg_size = OP_SIZE_LONG;   break;
                case 's': case 'S': info.arg_size = OP_SIZE_SINGLE; break;
                case 'x': case 'X': info.arg_size = OP_SIZE_XTND;   break;
                case 'p': case 'P': info.arg_size = OP_SIZE_PACKED; break;
                case 'w': case 'W': info.arg_size = OP_SIZE_WORD;   break;
                case 'd': case 'D': info.arg_size = OP_SIZE_DOUBLE; break;
                case 'b': case 'B': info.arg_size = OP_SIZE_BYTE;   break;
                default:
                    x3::_pass(ctx) = false;
                    return;
            }

            // return parsed "width" to improve diagnostic
            x3::_val(ctx) =  { insn_tok, info, *width };
        }
       
        // else bare opcode. no width specified
        else 
        {
            info.arg_size = OP_SIZE_VOID;           // no parsed size
            x3::_val(ctx) = {insn_tok, info, {} };  // value is <insn, info>
        }
    };

// create `kas_token` to parse m68k width (.W or W)
using  tok_m68k_insn_width = token_defn_t<KAS_STRING("M68K_INSN_WIDTH")>;
auto const m68k_insn_width = token<tok_m68k_insn_width>[-char_('.') >> alpha];

// M68K encodes several "options" in insn name. Decode them.
// invalid options error out in `m68k_insn_t::validate_args` 
auto const parse_insn = rule<class _, std::tuple<parser::kas_token
                                               , m68k_stmt_info_t
                                               , kas_position_tagged>> {} =
        lexeme[(m68k_insn_x3()      // insn_base_name
            >> -m68k_ccode::x3()    // optional condition-code
            >> -m68k_insn_width     // optional width
            >> !graph               // end token
            )[gen_stmt]
            ];

// Parser external interface
auto const m68k_stmt_def = (parse_insn > m68k_args)[m68k_stmt_t()];  

// Boilerplate to define `stmt` parser
m68k_stmt_x3 m68k_stmt {"m68k_stmt"};
BOOST_SPIRIT_DEFINE(m68k_stmt)
}


#endif
