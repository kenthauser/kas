#ifndef KAS_EXPR_H
#define KAS_EXPR_H

// Expression subsystem interface

// Define the expression variant: `expr_t`
// Declare the interface to the expression parser
#include "expr_variant.h"
#include "parser/parser_context.h"
#include "parser/kas_token.h"

namespace kas
{
namespace x3 = boost::spirit::x3;

// expose expression variant in top-level namespace
using expression::ast::expr_t;

// declare `expr()` interface
namespace expression::parser
{
    using namespace kas::parser;
    
    // NB: _DECLARE macro just declares a templated `parse_rule` function
    // NB: this rule is instantiated with standard context in `expr.cc`
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
            attribute_type tok;
            
            // parse input in "standard" context
            kas_context ctx(context);
            bool result = x3::as_parser(expr_type()).parse(first, last, ctx(), unused, tok);
            
            // if parsed correctly, update attribute from parsed value
            if (result)
                x3::traits::move_to(tok, attr);

            // true if `expr` parsed successfully
            return result;
        }
    };
}

// expose `expr` interface at top level
using expression::parser::expr;

}

#endif
