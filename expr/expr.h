#ifndef KAS_EXPR_H
#define KAS_EXPR_H

// Expression subsystem interface
//
// Define the expression variant: `expr_t`
// Declare the interface to the expression parser

#include "expr_variant.h"
#include "parser/parser_config.h"
#include "parser/token_parser.h"

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
    //using expr_type = x3::rule<class tag_expr, expr_t>;
    using expr_type = x3::rule<class tag_expr, kas_token>;
    BOOST_SPIRIT_DECLARE(expr_type)
    
    // Declare an "expression" parser hook.
    struct expr : x3::parser<expr>
    {
        using attribute_type = kas_token;

        template <typename Iterator, typename Context, typename Attribute>
        bool parse(Iterator& first, Iterator const& last,
                    Context const& context, x3::unused_type unused, Attribute& attr) const
        {
            attribute_type e;
            
            // need a standardized context for multi-file Instantiation
            auto& skipper  = x3::get<x3::skipper_tag>(context);
            auto& ehandler = x3::get<kas::parser::error_handler_tag>(context);
            auto& diag     = x3::get<kas::parser::error_diag_tag>(context);
            
            // rebuild
            auto skipper_ctx = x3::make_context<x3::skipper_tag>(skipper);
            auto diag_ctx    = x3::make_context<error_diag_tag>(diag, skipper_ctx);
            expr_context_type ctx = x3::make_context<error_handler_tag>(ehandler, diag_ctx);
            
            bool result = as_parser(expr_type()).parse(first, last, ctx, unused, e);
            
            //if (result)
            //    x3::traits::move_to(e, attr);
            return result;
        }
    };
}

// expose `expr` interface at top level
using expression::parser::expr;

}

#endif
