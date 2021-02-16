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
// Step 3. Test if `op` is a binary division op (from trait).
//         If so, test divisor for zero.
//
// Step 4. Evaluate & return.

#include "expr.h"
#include "expr_op_types.h"
#include "parser/kas_error.h"

namespace kas::expression::detail
{

struct token_op_eval
{
    using result_type = parser::kas_token;

    // NB: this is where array of `tokens` is evaluated
    template <typename...Ts>
    result_type operator()(Ts&&...args) const noexcept
    {
        static_assert(sizeof...(Ts) <= expr_op::MAX_ARITY
                     , "expression exceeds expr_op::MAX_ARITY");

        auto h = key(tokens.data(), sizeof...(Ts));
        return op.eval<sizeof...(Ts)>(op_loc, tokens.data(), h, {args...});
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
                    , KEY_T key
                    , EVAL::exec_arg_t<N>&& args
                    ) const noexcept 
{
    // set token location pointer from input tokens
    auto tag = [this, op_loc, tokens](kas_position_tagged &loc)
        {
            // ARITY > 1 : loc is from first arg to last
            if constexpr (N > 1)
            {
                loc = {*tokens[0], *tokens[N-1]};
            }

            // unary operation: tag according to operation type
            if constexpr (N == 1)
            {
                switch (priority())
                {
                    case PRI_PFX:
                        loc = {op_loc, *tokens[0]};
                        break;
                    case PRI_SFX:
                        loc = {*tokens[0], op_loc};
                        break;
                    default:
                        loc = op_loc;
                        break;
                }
            }
        };

    // test if args match operator
    auto it = ops.find(key);
#ifdef EXPR_TRACE_EVAL
    std::cout << "expr_op::eval: oper = " << op_loc.where();
    std::cout << ", key = " << std::hex << key;
    std::cout << ", supported = " << std::boolalpha << (it != ops.end()) << std::endl;
#endif
    if (it == ops.end())
    {
        // propogate error if previously detected
        constexpr auto Err_Index = expr_t::index<kas::parser::kas_diag_t>();
        for (auto i= 0; i < N; ++i)
            if (tokens[i]->expr_index() == Err_Index)
                return *tokens[i];

        // create location object for diagnostic.
        kas_position_tagged loc;
        tag(loc);
        return kas::parser::kas_diag_t::error("Invalid expression", loc);
    }   
    
    // look for divide by zero if ARITY == 2
    if constexpr (N == 2)
    {
        // only need `is_zero` functions for ARITY == 2
        static constexpr zero_fn_t<expr_types> zero_fns;
        auto dem_type = tokens[1]->expr_index();

        if (defn_p->is_divide)
            if (zero_fns[dem_type-1](args[1]))
                return kas::parser::kas_diag_t::error("Divide by zero", *tokens[1]);
    }

    // evaluate (function pointer retrieved from key table)
    kas_token tok = it->second(std::move(args));
    tag(tok);
    return tok;
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
            print_expr_op_args(args...);
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
    // create evalutaion "object' with all `kas_position_tagged` locations
    auto obj    = token_op_eval{*this, loc, {&args...}};
    auto result = obj(args()...);       // evalutate `expr_op`

#ifdef EXPR_TRACE_EVAL
    std::cout << "result: " << result << std::endl;
#endif

    return result;
}
}

#endif
