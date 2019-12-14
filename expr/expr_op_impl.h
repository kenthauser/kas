#ifndef KAS_EXPR_OP_IMPL_H
#define KAS_EXPR_OP_IMPL_H

// Visitor function to evaluate operators
//
// It's less complicated than it first looks:
//
// Step 1. Look at arg types to find a error. If so,
//         propogate first error as exprssion value.
//
// Step 2. See if argument types are compatible with
//         operation (ie what does `result_of` think).
//         If no match, return "Invalid Expression"
//
// Step 3. Test if `op` is a division op (from trait).
//         If so, test divisor for zero.
//
// Step 4. Evaluate & return.

#include "expr.h"
#include "expr_op_types.h"
#include "parser/kas_error.h"

namespace kas::expression::detail
{

// look for error type in argument list (meta::npos if none)
template <typename...Ts>
using Err_Index = meta::find_index<meta::list<Ts...>, kas::parser::kas_diag_t&>;

struct expr_op_visitor
{
    using result_type  = parser::kas_token;

    // NB: visitor replaces "called" types with "visited" types
    // NB: this is where array of `exprs` turns into array of `types`
    template <typename...Ts>
    result_type operator()(Ts&&...args) const noexcept
    {
        static_assert(sizeof...(Ts) <= expr_op::MAX_ARITY
                     , "expression exceeds expr_op::MAX_ARITY");

        // scan args for `e_error_t` 
        constexpr auto err   = Err_Index<Ts...>::value;

        // divide_by_zero needs type_index of denominator for ARITY of 2
        // NB: make sure list has at least two types
        constexpr auto dem_type = expr_index<meta::at_c<meta::list<Ts..., void, void>, 1>>;
        constexpr auto h = hash<Ts...>();
        return op.eval<sizeof...(Ts)>(op_loc, tokens.data(), h, {&args...}, err, dem_type);
    }

    // references to operator & parsed location
    expr_op const &op;
    kas_position_tagged const& op_loc;

    // pointers to tokens (including locations)
    std::array<result_type const *, expr_op::MAX_ARITY> tokens;
};


template <typename> struct zero_fn_t;

template <typename...Ts>
struct zero_fn_t<list<Ts...>>
{
    using fn_t = bool(*)(void const *);

    static constexpr fn_t fns[] { is_zero<Ts>::value... };

    constexpr auto operator[](std::size_t n) const
    {
        return fns[n];
    }
};


template <std::size_t N>
parser::kas_token expr_op::eval(kas_position_tagged const& op_loc
                    , parser::kas_token const* const* tokens
                    , HASH_T hash
                    , EVAL::exec_arg_t<N>&& args
                    , std::size_t err
                    , std::size_t dem_type
                    ) const noexcept 
{
    // propogate error if previously detected
    if (err != meta::npos::value)
        return *tokens[err];

    // test if args match operator
    auto it = ops.find(hash);

    if (it == ops.end())
        return {kas::parser::kas_diag_t::error("Invalid expression", op_loc), op_loc};
        
    // look for divide by zero if ARITY == 2
    if constexpr (N == 2)
    {
        // only need `is_zero` functions for ARITY == 2
        static constexpr zero_fn_t<expr_types> zero_fns;
        
        if (defn_p->is_divide)
            if (zero_fns[dem_type-1](args[1]))
            {
            #if 1
                // XXX `op_loc` is zero?
                //return {kas::parser::kas_diag_t::error("Divide by zero", op_loc), op_loc, *tokens[1]};
                return {kas::parser::kas_diag_t::error("Divide by zero", *tokens[1]), *tokens[1]};
            #else
                auto& err = kas::parser::kas_diag_t::error("Divide by zero", op_loc);
                std::cout << "eval: err = " << expr_t(err) << std::endl;
                expr_t e (err);
                std::cout << "eval: e = " << e << std::endl;
                parser::kas_token tok = { err.ref(), op_loc, *tokens[1]};
                std::cout << "eval: tok = " << tok << std::endl;
                return tok;
            #endif
            }
    }

    // evaluate (function pointer retrieved from hash table)
    auto e = it->second(std::move(args));

    // set token location pointer from input tokens
    // ARITY > 1 : loc is from first arg to last
    if constexpr (N > 1)
        return { e, *tokens[0], *tokens[N-1] };
    // unary prefix : loc is op + arg
    if constexpr (N == 1)
        return { e, op_loc,  *tokens[0] };
    
    // XXX: unary suffix not yet supported
}

#ifdef EXPR_TRACE_EVAL
namespace 
{
    template <typename T, typename...Ts>
    void print_expr_op_args(T&& arg, Ts&&...args)
    {
        std::cout << arg;
        if constexpr (sizeof...(Ts))
        {
            std::cout << ", ";
            print_expr_op_args(std::forward<Ts>(args)...);
        }
    }
}
#endif

template <typename...Ts>
parser::kas_token expr_op::operator()(kas_position_tagged const& loc, Ts&&...args) const noexcept 
{
#ifdef EXPR_TRACE_EVAL
    std::cout << "eval: " << name() << " (";
    print_expr_op_args(std::forward<Ts>(args)...);
    std::cout << ")" << std::endl;
#endif

    // create "visitor" with all `kas_position_tagged` locations
    auto vis = expr_op_visitor{*this, loc, {&args...}};
    
    // NB: can't take address of r-values. Thus, don't std::forward<> args
    auto result = boost::apply_visitor(vis)(args.raw_expr()...);

#ifdef EXPR_TRACE_EVAL
    std::cout << "result: " << result << std::endl;
#endif

    return result;
}

}

#endif
