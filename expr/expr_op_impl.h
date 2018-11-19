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

#include "utility/string_mpl.h"
#include "utility/print_type_name.h"

namespace kas::expression::detail
{

// look for error type in argument list (meta::npos if none)
template <typename...Ts>
using Err_Index = meta::find_index<meta::list<Ts...>, kas::parser::kas_diag&>;

struct op_visitor
{
    using result_type  = expr_t;

    template <typename...Ts>
    result_type operator()(Ts&&...args) const noexcept
    {
        // scan args for `e_error_t` 
        constexpr auto err   = Err_Index<Ts...>::value;

        // divide_by_zero needs type_index of denominator for ARITY of 2
        // NB: make sure list has at least two types
        constexpr auto dem_type = expr_index<meta::at_c<meta::list<Ts..., void, void>, 1>>;
        constexpr auto h = hash<Ts...>();
        return op.eval<sizeof...(Ts)>(loc, h, {&args...}, err, dem_type);
    }

    expr_op const &op;
    kas_position_tagged const& loc;
};

template <typename T, typename = void>
struct has_set_loc : std::false_type {};

template <typename T>
struct has_set_loc<T, std::void_t<decltype(&std::remove_reference_t<T>::set_loc)>>
                    : std::true_type {};

void set_loc(expr_t& e, kas_position_tagged const& loc)
{
    // don't "dereference" ref_loc_t<> containers
    e.base_type::apply_visitor(x3::make_lambda_visitor<void>(
        [&loc](auto& node) -> 
            std::enable_if_t<has_set_loc<decltype(node)>::value>
                { node.set_loc(loc); },
        [&loc](auto& node) ->
            std::enable_if_t<!has_set_loc<decltype(node)>::value>
                {}
     ));
}

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
expr_t expr_op::eval(kas_position_tagged const& loc
                    , HASH_T hash
                    , EVAL::exec_arg_t<N>&& args
                    , std::size_t err
                    , std::size_t dem_type
                    ) const noexcept 
{
    // propogate error if previously detected
    if (err != meta::npos::value)
        return *static_cast<kas::parser::kas_diag const *>(args[err]);

    // test if args match operator
    auto it = ops.find(hash);
    if (it == ops.end())
        return kas::parser::kas_diag::error("Invalid expression", loc);
        
    // look for divide by zero if ARITY == 2
    if constexpr (N == 2)
    {
        // only need `is_zero` functions for ARITY == 2
        static constexpr zero_fn_t<expr_types> zero_fns;
        
        if (defn_p->is_divide)
            if (zero_fns[dem_type-1](args[1]))
                return kas::parser::kas_diag::error("Divide by zero", loc);
    }

    // evaluate 
    auto e = it->second(std::move(args)); 
    set_loc(e, loc);
    return e;
}


template <typename...Ts>
expr_t expr_op::operator()(kas_position_tagged const& loc, Ts&&...args) const noexcept 
{
    auto vis = op_visitor{*this, loc};
    // NB: can't take address of r-values. Thus, don't std::forward<> args
    return boost::apply_visitor(vis)(args...);
}

}

#endif
