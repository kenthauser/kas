#ifndef KAS_BSD_BSD_EXPR_TYPES_H
#define KAS_BSD_BSD_EXPR_TYPES_H

#include "expr/expr_types.h"
#include "expr/precedence.h"
#include "kas_core/core_types.h"
#include "parser/parser_types.h"
#include "bsd_arg_defn.h"
#include "expr/precedence.h"

// include default parsers
#include "expr/c_int_parser.h"
#include "expr/c_float_parser.h"
#include "expr/c_string_parser.h"

#include <boost/spirit/home/x3.hpp>
#include <boost/mpl/string.hpp>

namespace kas::bsd::parser
{
// add namespace lookups for `bsd::parser`
namespace x3 = boost::spirit::x3;
using namespace kas::parser;
}


namespace kas::bsd::parser::bnf
{
// don't need to annotate location, since parsed as "tokens"
using dot_parser_x3 = x3::rule<struct _bsd_dot, kas_token>;
BOOST_SPIRIT_DECLARE(dot_parser_x3)

using sym_parser_x3 = x3::rule<struct _bsd_sym, kas_token>;
BOOST_SPIRIT_DECLARE(sym_parser_x3)
}

namespace kas::parser::detail
{

// declare default BSD comment & separator values
template<typename = void> struct fmt_separator_str : KAS_STRING("@") {};
template<typename = void> struct fmt_comment_str   : KAS_STRING(";") {};
}

// BSD doesn't define new types to add, just parsers for existing types
namespace kas::expression::detail
{
using namespace meta;

// default: use c-language parsers for BSD
template<> struct fixed_p<void>  : id<quote<literal::c_int_parser>>    {};
template<> struct float_p<void>  : id<quote<literal::c_float_parser>>  {};
template<> struct string_p<void> : id<quote<literal::c_string_parser>> {};

//template <> struct float_value<void> : id<double> {};

// NB: expr looks in reverse order of definition.
//     Need `dot` to be searched before `sym` to
//     prevent "." being parsed as symbol
template<> struct term_parsers_v<defn_fmt> : meta::list<
                         bsd::parser::bnf::sym_parser_x3
                       , bsd::parser::bnf::dot_parser_x3
                       > {};

// declare bsd operator precedence rules:
// 1. prefix/suffix above mult
// 2. mult above add
// 3. all others equal to add
struct precedence_bsd
{
    using prec_t = expression::precedence::prec_t;
   
    static prec_t value(prec_t pri)
    {
        if (pri > precedence::PRI_PRE_MULT)
            return pri;
        if (pri > precedence::PRI_PRE_ADD)
            return precedence::PRI_PRE_MULT;
        return precedence::NUM_PRI;
    }
};
template<> struct e_precedence<void> : id<precedence_bsd> {};
}

#endif

