#ifndef KAS_BSD_BSD_EXPR_TYPES_H
#define KAS_BSD_BSD_EXPR_TYPES_H

// XXX delete many includes...
#include "expr/expr_types.h"
#include "kas_core/core_types.h"
#include "parser/parser_types.h"
#include "expr/literal_types.h"

#include <boost/spirit/home/x3.hpp>
#include <boost/mpl/string.hpp>

namespace kas::bsd::parser
{
namespace x3 = boost::spirit::x3;

// don't need to annotate location, since parsed as "tokens"
using dot_parser_x3 = x3::rule<struct _, core::addr_ref>;
BOOST_SPIRIT_DECLARE(dot_parser_x3)

using sym_parser_x3 = x3::rule<struct _, core::symbol_ref>;
BOOST_SPIRIT_DECLARE(sym_parser_x3)

#if 0    
// don't need to annotate location, since parsed as "tokens"
using X_dot_parser_x3 = x3::rule<struct _tag_dot, parser::kas_token>;
BOOST_SPIRIT_DECLARE(X_dot_parser_x3)

using X_sym_parser_x3 = x3::rule<struct _tag_sym, parser::kas_token>;
BOOST_SPIRIT_DECLARE(X_sym_parser_x3)
#endif
}

namespace kas::parser::detail
{
    // declare default BSD comment & separator values
    template<typename = void> struct fmt_separator_str : boost::mpl::string<'!'> {};
    template<typename = void> struct fmt_comment_str   : boost::mpl::string<';'> {};
}

// BSD doesn't define new types to add, just parsers
namespace kas::expression
{
// use general representation for strings 
//template <> struct e_string<void> : meta::id<kas_string> {};
//template <> struct e_float<void>  : meta::id<kas_float>  {};

namespace detail
{
    // NB: expr looks in reverse order of definition.
    //     Need `dot` to be searched before `sym` to
    //     prevent "." being parsed as symbol
    template<> struct term_parsers_v<defn_fmt> : meta::list<
                             bsd::parser::sym_parser_x3
                           , bsd::parser::dot_parser_x3
                           > {};
}
}


#endif
