#ifndef KAS_EXPR_H
#define KAS_EXPR_H

// Expression subsystem interface
//
// Define the expression variant: `expr_t`
// Declare the interface to the expression parser

#include "expr_variant.h"
#include <boost/spirit/home/x3.hpp>


namespace kas
{
namespace x3 = boost::spirit::x3;

// expose expression variant in top-level namespace
using expression::ast::expr_t;

namespace expression::parser
{
    using namespace kas::parser;
    
    // NB: _DECLARE macro just declares a templated `parse_rule` function
    // NB: this rule is instantiated with standard context in `expr.cc`
    using expr_type = x3::rule<class tag_expr, expr_t>;
    BOOST_SPIRIT_DECLARE(expr_type)
    
    // Declare an "expression" parser hook.
    struct expr : x3::parser<expr>
    {
        using attribute_type = expr_t;

        template <typename Iterator, typename Context, typename Attribute>
        bool parse(Iterator& first, Iterator const& last,
                    Context const& context, x3::unused_type unused, Attribute& attr) const
        {
            expr_t e;
            
            // need a standardized context
            // XXX add paraser/parser_types.h support
            auto& skipper  = x3::get<x3::skipper_tag>(context);
            auto& ehandler = x3::get<kas::parser::error_handler_tag>(context);
            
            auto skipper_ctx = x3::make_context<x3::skipper_tag>(skipper);
            auto ctx = x3::make_context<kas::parser::error_handler_tag>(ehandler, skipper_ctx);
            
            bool result = as_parser(expr_type()).parse(first, last, ctx, unused, e);
            
            if (result)
                x3::traits::move_to(e, attr);
            return result;
        }
    };
}

// expose `expr` interface at top level
using expression::parser::expr;

}

#endif
