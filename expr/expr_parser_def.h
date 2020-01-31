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
    // leave `opers` in container (with execution methods)
    template <typename OPER_PAIR_T>
    struct shunting_stack
    {
        template <typename U>
        shunting_stack(U&& initial) : tokens{ std::forward<U>(initial) } {}

        void push(OPER_PAIR_T&& t)
        {
            // split pair onto two stacks: tokens & operators
            tokens.push_back(std::move(t.second));
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

        bool operator<=(OPER_PAIR_T const& node) const
        {
            auto get_priority = [](auto& node)
                { return tok_oper_bin(node.first)()->priority(); };

            auto priority = get_priority(node);
            return !empty() && get_priority(opers.back()) <= priority;
        }

        bool   empty()  const { return opers.empty(); }
        auto&  result()       { return tokens.back(); }

    private:
        // `opers` vector holds `op_tokens_pair_t` instances
        std::vector<OPER_PAIR_T> opers;
        std::vector<kas_token>   tokens;
    };
};


//////////////////////////////////////////////////////////////////////////
//  Expression Parser Definition
//////////////////////////////////////////////////////////////////////////

// Parse operator expression pairs into a struct before evaluating.
// Pairs are oper/token for binary & pfx. Pair is token/oper for sfx.
struct op_expr_pair_t
{
    // for `bin_op`, hook into `expr_op` evalution machinery
    template <typename...Ts>
    kas_token operator()(Ts&&...args) 
    {
        if (auto oper_p = tok_oper_bin(first)())
            return (*oper_p)(first, std::forward<Ts>(args)...);
        
        return bad_oper();
    }

    // evaluate `pfx_op` and `sfx_op` pairs
    operator kas_token() 
    {
        // check for prefix unary op
        if (auto oper_p = tok_oper_pfx(first)())
            return (*oper_p)(first, second);
        // check for suffix unary op
        if (auto oper_p = tok_oper_sfx(second)())
            return (*oper_p)(second, first);
        
        // internal logic error
        return bad_oper();
    }

private:
    kas_token bad_oper() const
    {
        std::string msg{"Internal::invalid operator: "};
        msg += first.name();
        msg += ", ";
        msg += second.name();
    
        throw std::logic_error{msg};
    }

public:
   kas_token   first, second;
};

// rules for expressions and terminals
expr_type                         expr   = "expr";
x3::rule<class _inner, kas_token> inner  = "inner";
x3::rule<class _term , kas_token> term   = "term";

// rules to parse operators
x3::rule<class bin_op, op_expr_pair_t> bin_op = "bin_op";
x3::rule<class pfx_op, op_expr_pair_t> pfx_op = "pfx_op";
x3::rule<class sfx_op, op_expr_pair_t> sfx_op = "sfx_op";

// Declare primary/unary/binary subpressions
auto const bin_op_def = tok_bin_op >> term;
auto const pfx_op_def = tok_pfx_op >  term;
auto const sfx_op_def = term > sfx_op_x3;
BOOST_SPIRIT_DEFINE(bin_op, pfx_op, sfx_op)

// Parser "terminal" expressions
auto inline term_op_p()
{
    // test parsers in reverse order (eg: try `dot` before `float` before `fixed`)
    return combine_parsers(meta::reverse<term_parsers>());
}

#if 0
// XXX rule creates new context which causes link errors...
// create `paren_op` rule so that parenthesis are added to parsed token
struct _tag_paren : annotate_on_success {};
auto const paren_op  = x3::rule<_tag_paren, kas_token> {"paren"} =
                            '(' > inner > ')';
#else
// XXX create "tag" lambda for update...
auto const paren_op = '(' > inner > ')';
#endif


// Combine above subexpressions into full expression parsing
// NB: enabling sfx_op screws up "MISSING" parsing.
auto const term_def  = term_op_p() | pfx_op /* | sfx_op */ | paren_op;
auto const inner_def = (term > *bin_op)[shunting_yard()];
auto const expr_def  = inner;

BOOST_SPIRIT_DEFINE(expr, inner, term)
}

// boost::spirit boilerplate for parsing pairs
#include <boost/fusion/include/adapt_struct.hpp>

BOOST_FUSION_ADAPT_STRUCT(kas::expression::parser::op_expr_pair_t, first, second)

#endif
