#ifndef KAS_EXPR_PARSER_DEF_H
#define KAS_EXPR_PARSER_DEF_H

//
// The actual boost::spirit parser for expressions
//
// Implement the `shunting yard` algorithm to process precedence
//


#include "expr.h"                   // defines ast & parser terminals
#include "operators.h"              // defines expression operations
#include "parser/parser.h"

#include "parser/error_handler_base.h"
#include "parser/annotate_on_success.hpp"
#include "parser/token_parser.h"
#include "parser/combine_parsers.h"

#include <boost/spirit/home/x3.hpp>

namespace kas::expression::parser
{
namespace x3 = boost::spirit::x3;
namespace ascii = boost::spirit::x3::ascii;

//////////////////////////////////////////////////////////////////////////
// Shunting yard algorithm (see Wikipedia) to resolve operator precedence
//////////////////////////////////////////////////////////////////////////
    
struct shunting_yard
{
    template <typename Context>
    void operator()(Context const& ctx) const
    {
        // grab args & types from context (b_nodes is a std::vector<T>)
        auto& args    = x3::_attr(ctx);
        auto& initial = boost::fusion::at_c<0>(args);
        auto& b_nodes = boost::fusion::at_c<1>(args);

        // perform "forward declaration" of `op_expr_pair_t`
        using b_node_t = typename std::decay_t<decltype(b_nodes)>::value_type;

        // short-circuit for common case of no bin_ops
        if (b_nodes.empty())
        {
            x3::_val(ctx) = std::move(initial);
            return;
        }

        // implement shuting yard algorithm
        shunting_stack<b_node_t> ops{ std::move(initial) };
        for (auto&& node : b_nodes)
        {
            while (ops <= node)
                ops.pop();
            ops.push(std::move(node));
        }

        // clear stack & return result
        while (!ops.empty())
            ops.pop();
        x3::_val(ctx) = ops.result();
    }

    // implement shunting yard stack opersations
    // extract `tokens` from pair & push on separate stack
    // leave `opers` in position-tagged container
    template <typename OPER_T>
    struct shunting_stack
    {
        template <typename U>
        shunting_stack(U&& initial) : tokens{ std::forward<U>(initial) } {}

        void push(OPER_T&& t)
        {
            // split pair onto two stacks: tokens & operators
            tokens.push_back(std::move(t.token));
            opers.push_back(std::move(t));
        }

        // NB: this is where binary operator evauluation actually occurs
        void pop()
        {
            // reference top tokens on stack
            auto  end_iter  = tokens.end();
            auto& top_token = *--end_iter;
            auto& dest      = *--end_iter;
            dest = opers.back()(std::move(dest), std::move(top_token));
            tokens.pop_back();      // remove consumed object 
            opers.pop_back();       // remove consumed object
        }

        bool operator<=(OPER_T const& node) const
        {
            auto priority = node.oper->priority();
            return !empty() && opers.back().oper->priority() <= priority;
        }

        bool   empty()  const { return opers.empty();  }
        auto&  result()       { return tokens.back();  }

    private:
        // `opers` vector holds `op_tokens_pair_t` instances
        std::vector<OPER_T>    opers;
        std::vector<kas_token> tokens;
    };
};


//////////////////////////////////////////////////////////////////////////
//  Expression Parser Definition
//////////////////////////////////////////////////////////////////////////

x3::rule<class tag_expr, kas_token> expr  = "expr";
x3::rule<class inner   , kas_token> inner = "expr_inner";
x3::rule<class term    , kas_token> term  = "term";

// default implementation of quoted string parser
detail::dflt_string_p<std::string> qs;// = "quoted string";
auto const qs_def = x3::lexeme['"' >> *(x3::char_ - '"') >> '"' ];
BOOST_SPIRIT_DEFINE(qs)

// Parse operator expression pairs into a struct before evaluating.
// This allows the parsed struct to be position_tagged for
// error reporting.
struct op_expr_pair_t : kas_position_tagged
{
    op_expr_pair_t() = default;
    op_expr_pair_t(expr_op const *op, kas_token const& e)
            : oper(op), token(e) {}

    // hook into `expr_op` evalution machinery
    template <typename...Ts>
    kas_token operator()(Ts&&...args) const
    {
        // `*this` is for operator location
        return (*oper)(*this, std::forward<Ts>(args)...);
    }

    // evaluate `pfx_op` and `sfx_op` pairs
    operator kas_token()
    {
        return (*oper)(*this, token);
    }
    
    expr_op const *oper;
    kas_token      token;
};

// reverse arg order for suffix pairs
struct sfx_expr_pair_t : op_expr_pair_t
{
    sfx_expr_pair_t() = default;
    sfx_expr_pair_t(kas_token e, expr_op const *op)
        : op_expr_pair_t(op, e) {}
};

// rules for primary expressions & operator "pairs"
x3::rule<class p_expr, kas_token>       p_expr = "p_expr";
x3::rule<class bin_op, op_expr_pair_t>  bin_op = "bin_op";
x3::rule<class pfx_op, op_expr_pair_t>  pfx_op = "pfx_op";
//x3::rule<class sfx_op, sfx_expr_pair_t> sfx_op = "sfx_op";

#if 0
// XXX rule creates new context which causes link errors...
// create `paren_op` rule so that parenthesis are added to parsed token
struct _tag_paren : annotate_on_success {};
auto const paren_op  = x3::rule<_tag_paren, kas_token> {"paren"} = 
                            '(' > inner > ')';
#else
auto const paren_op = '(' > inner > ')';
#endif

// Declare primary/unary/binary subpressions

auto inline term_op_p()
{
    // test parsers in reverse order (eg: try `dot` before `float` before `fixed`)
    using parser_tuple = meta::apply<meta::quote<std::tuple>, meta::reverse<term_parsers>>;
    return combine_parsers(parser_tuple());
}

auto const bin_op_def = bin_op_x3 >> term;
auto const pfx_op_def = pfx_op_x3 >  term;
//auto const sfx_op_def = term > sfx_op_x3;

// Combine above subexpressions into full expression parsing
// NB: enabling sfx_op screws up "MISSING" parsing.
auto const term_def = term_op_p() | pfx_op /* | sfx_op */ | paren_op;
auto const inner_def = (term > *bin_op)[shunting_yard()];
auto const expr_def = inner;

BOOST_SPIRIT_DEFINE(expr, inner, term)
BOOST_SPIRIT_DEFINE(bin_op, pfx_op)

///////////////////////////////////////////////////////////////////////////
// Annotation and Error handling
///////////////////////////////////////////////////////////////////////////

// error handling only for outermost expression
struct tag_expr : kas::parser::error_handler_base {};
}

// boost::spirit boilerplate for parsing pairs
#include <boost/fusion/include/adapt_struct.hpp>

BOOST_FUSION_ADAPT_STRUCT(kas::expression::parser::op_expr_pair_t , oper, token)
BOOST_FUSION_ADAPT_STRUCT(kas::expression::parser::sfx_expr_pair_t, token, oper)

#endif
