#ifndef KAS__MIT_PARSER_DEF_H
#define KAS__MIT_PARSER_DEF_H

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

#include "parser/error_handler_base.h"
#include "m68k_parser_support.h"
#include "expr/expr.h"              // expression public interface

#include <boost/spirit/home/x3.hpp>


namespace kas { namespace m68k { namespace parser
{
    namespace x3 = boost::spirit::x3;
    using namespace x3;

    //////////////////////////////////////////////////////////////////////////
    //  M68K Instruction Parser Definition
    //////////////////////////////////////////////////////////////////////////

    x3::rule<class m68k_stmt,   stmt_t>     m68k_stmt       = "m68k_stmt";
    x3::rule<class m68k_arg,    m68k_arg_t> m68k_arg        = "m68k_arg";
    x3::rule<class m68k_bf_arg, m68k_arg_t> m68k_bf_arg     = "m68k_bf_arg";
    x3::rule<class m68k_stmt,   stmt_m68k>  m68k_basic_stmt = "m68k_basic_stmt";


    //////////////////////////////////////////////////////////////////////////
    // Parse M68K argument using MIT syntax
    //////////////////////////////////////////////////////////////////////////

    x3::rule<class _mit_arg,  m68k_parsed_arg_t> const mit_parsed_arg = "mit_parsed_arg";
    // x3::rule<class _mit_bf_p, m68k_displacement> const mit_bitfield  = "mit_bitfield";
    // x3::rule<class _mit_bf,   m68k_displacement> const mit_parsed_bf = "mit_parsed_bf";
    x3::rule<class _attr, m68k_displacement> const mit_arg_suffix = "mit_arg_suffix";
    x3::rule<class _mit_pair, m68k_displacement> const mit_pair_arg = "mit_pair_arg";
    x3::rule<class _expr_s_s, expr_size_scale> const mit_ss_p = "mit_ss_p";

   // an m68k argument is "expression" augmented by "address mode"
    auto m68k_disp = [](auto&& mode) { return attr(m68k_displacement{mode}); };
    auto z_expr    = [] { return attr(expr_size_scale{0, 0, P_SCALE_Z}); };


    auto const m68k_arg_def =  mit_parsed_arg;

    // auto const mit_bitfield = lit('{') >
    //                                 ((-lit('#') > expr()) % ',')
    //                             > lit('}') >> m68k_disp(PARSE_BITFIELD);

    // auto const mit_bitfield_def = lit('{') > attr(PARSE_BITFIELD) >
    //                                 ((-lit('#') > expr()) % ',')
    //                                 > lit('}');

    // auto const mit_parsed_bf_def = mit_bitfield;
    // auto const m68k_bf_arg_def   = mit_parsed_bf;

    auto const mit_size =
              no_case[":w"] > attr(M_SIZE_WORD)
            | no_case[":l"] > attr(M_SIZE_LONG)
            | eps           > attr(M_SIZE_AUTO)
            ;
    auto const mit_scale =
              ":1" > attr(P_SCALE_1)
            | ":2" > attr(P_SCALE_2)
            | ":4" > attr(P_SCALE_4)
            | ":8" > attr(P_SCALE_8)
            | eps  > attr(P_SCALE_AUTO)
            ;

    auto const mit_ss_p_def =
              expr()         > mit_size          > mit_scale
            ;

    auto const mit_parsed_arg_def =
              ( mit_ss_p        >> mit_arg_suffix)
            | '#' > mit_ss_p    > m68k_disp(PARSE_IMMED)
            ;

    auto const mit_indirect_arg = lit('@') >> ('(' > mit_ss_p % ',' > ')');

    auto const mit_pair_arg_def  = repeat(1)[mit_ss_p];

    auto const mit_arg_suffix_def =
              attr(PARSE_INDIR)  >> +mit_indirect_arg
            | "@+"         >> m68k_disp(PARSE_INCR)
            | "@-"         >> m68k_disp(PARSE_DECR)
            | "@"          >> m68k_disp(PARSE_INDIR)
            | !lit(':')     > m68k_disp(PARSE_DIR)
            ;


    BOOST_SPIRIT_DEFINE(m68k_arg)
    BOOST_SPIRIT_DEFINE(mit_pair_arg)
    // BOOST_SPIRIT_DEFINE(m68k_bf_arg)

    // an m68k instruction is "opcode" followed by comma-separated "arg_list"
    // bitfield breaks regularity. Allow bitfield to follow arg w/o comma
    auto const m68k_args = m68k_arg % ',';

    // auto const m68k_args = rule<class _args, std::list<m68k_arg_t>> {}
    //     // = m68k_arg >> *(lit(',') >> m68k_arg);
    //     = m68k_arg % ',';


    // auto const m68k_args = m68k_arg >> *(',' >> m68k_arg);// | m68k_bf_arg);
        // BOOST_SPIRIT_INSTANTIATE(m68k_args);

    auto const m68k_basic_stmt_def = m68k_opcode_parser() > m68k_args;
    auto const m68k_stmt_def = m68k_basic_stmt;

    BOOST_SPIRIT_DEFINE(m68k_stmt)
    BOOST_SPIRIT_DEFINE(m68k_basic_stmt)

    // BOOST_SPIRIT_DEFINE(mit_bitfield)

    BOOST_SPIRIT_DEFINE(mit_parsed_arg)
    // BOOST_SPIRIT_DEFINE(mit_parsed_bf)
    BOOST_SPIRIT_DEFINE(mit_arg_suffix)
    BOOST_SPIRIT_DEFINE(mit_ss_p)


}}}

namespace kas
{
    m68k::parser::m68k_stmt_type const& m68k_stmt()
    {
        using namespace m68k::parser;
        static m68k_stmt_type _stmt;
        return _stmt;
    }
}

#endif
