#ifndef KAS_EXPR_OP_EVAL_H
#define KAS_EXPR_OP_EVAL_H

// create instances to actually perform all `operations` on variant types
//
// only create instances for operator & argument tuples which are allowed.
//
// each `expr_op_eval` instance evaluates a single `operator, args...` tuple

#include "expr.h"
#include "kas_core/core_symbol.h"
#include "kas_core/core_addr.h"
#include "kas_core/core_expr_type.h"

using expr_op_return = kas::parser::kas_token;

namespace kas::expression::detail
{

struct expr_op_eval
{
    // args are stored in tuple
    template <std::size_t N>
    using exec_arg_t = std::array<void *, N>;

private:
    // Methods used to instantiate all of the opcode methods.
    // Types passed are all "bare" types without qualifiers
    // Args passed are all "void *", dereferenced to `T&` for evaluation
    template <typename OP, typename...Ts, std::size_t...Is>
    static decltype(auto) 
    do_exec(exec_arg_t<sizeof...(Ts)>&& args, std::index_sequence<Is...>)
    {
        return OP{}(*static_cast<Ts *>(std::get<Is>(args))...);
    }
    
    // evaluate OP with casted & dereferenced `void *` pointers
    // NB: third template arg for SFINAE support.
    template <typename OP, typename...Ts
            , typename RT = decltype(std::declval<OP>()(std::declval<Ts>()...))
            , typename    = std::enable_if_t<std::is_constructible_v<expr_op_return, RT>>>
    static expr_op_return
    gen_fn(exec_arg_t<sizeof...(Ts)>&& args)
    {
        using indexes = std::make_index_sequence<sizeof...(Ts)>;
        return do_exec<OP, Ts...>(std::move(args), indexes());
    }
   
public:
    // ctor: save pointer for [OP, list<types...>] pair
    template <typename OP, typename...Ts, typename = decltype(gen_fn<OP, Ts...>)>
    constexpr expr_op_eval(OP, meta::list<Ts...>) :      fn { gen_fn<OP, Ts...> } {}
    
    // ctor: invalid args to op -- clear fn
    template <typename...Ts>
    constexpr expr_op_eval(Ts...) : fn{} {}
    
    //
    // public interface
    //

    template <std::size_t N>
    auto operator()(exec_arg_t<N>&& args) const
    {
        return fn(std::move(args));
    }

    template <typename...Ts>
    auto operator()(Ts*...args) const
    {
        exec_arg_t<sizeof...(Ts)> arg_tuple{args...};
        return (*this)(std::move(arg_tuple));
    }
   
    // test if operation is allowed (constexpr)
    template <typename OP, typename T, typename = void>
    struct args_ok_impl : std::false_type {};

    template <typename OP, typename...Ts>
    struct args_ok_impl<OP, meta::list<Ts...>
                , std::void_t<decltype(gen_fn<OP, Ts...>)>> : std::true_type {};

    template <typename...Ts>
    using args_ok = _t<args_ok_impl<Ts...>>;

    template <typename OP, typename T>
    static constexpr auto ok(OP, T) { return args_ok<OP, T>::value; }


    // test if operation is allowed (not constexpr)
    operator bool() const { return fn; }

    // all ARITY limitations are in the union
    // NB: `MAX_ARITY` not actually inspected. Just comment.
    static constexpr auto MAX_ARITY = 3;

private:
    // store and dispach by argment arity via union
    union fn_ptrs
    {
        using RT = expr_op_return;
        
        // declare pointer to fn accepting `exec_arg_t`
        template <std::size_t N>
        using exec_fn_t = RT (*)(exec_arg_t<N>&&);

        constexpr fn_ptrs(void * = {})    : raw ()  {}
        constexpr fn_ptrs(exec_fn_t<1> p) : fn_1(p) {}
        constexpr fn_ptrs(exec_fn_t<2> p) : fn_2(p) {}
        constexpr fn_ptrs(exec_fn_t<3> p) : fn_3(p) {}

        template <std::size_t N>
        RT operator()(exec_arg_t<N>&& args) const
        {
            if constexpr (N == 1)
                return fn_1(std::move(args));
            else if constexpr (N == 2)
                return fn_2(std::move(args));
            else if constexpr (N == 3)
                return fn_3(std::move(args));
            else
                static_assert(N == 1, "invalid ARITY");
        }

        // test if fn pointer set (not constexpr)
        operator bool() const { return raw; }
    
    private:
        // one pointer per arity
        void const  *raw;
        exec_fn_t<1> fn_1;
        exec_fn_t<2> fn_2;
        exec_fn_t<3> fn_3;
    } fn;
};


// functions to generate a "HASH" to identify type tuple
using HASH_T = unsigned;

template <typename T, typename...Ts>
constexpr static HASH_T hash(HASH_T N = 0)
{
    // sanity check for HASH / ARITY
    constexpr auto H_SHIFT = 8;
    constexpr auto HASH_BITS  = std::numeric_limits<decltype(N)>::digits;

    if constexpr (((sizeof...(Ts) + 1) * H_SHIFT) > HASH_BITS)
        static_assert(sizeof...(Ts) == 0, "Invalid Hash for ARITY");

    // fold current `T` into hash
    N = (N << H_SHIFT) + expr_t::index<T>();

    // recurse as required
    if constexpr (sizeof...(Ts) == 0)
        return N;
    else
        return hash<Ts...>(N);
}

template <typename...Ts>
constexpr static auto hash_fn(meta::list<Ts...>)
{
    return hash<Ts...>();
}

// XXX quick & dirty equivalent fn. needs to be refactored
static HASH_T hash(kas_token const * const *tokens, std::size_t n)
{
    // sanity check for HASH / ARITY
    constexpr auto H_SHIFT = 8;

    HASH_T hash {};
    while (n--)
    {
        hash <<= H_SHIFT;
        hash += (*tokens++)->index();
    }
    return hash;
}

// generate list of TERMS for [OP, ARITY] 
using expr_types = concat<typename expr_t::plain, typename expr_t::unwrapped>;
template <typename OP, typename ARITY>
using expr_op_terms = filter<
                 cartesian_product<repeat_n<ARITY, expr_types>>
               , bind_front<quote<expr_op_eval::args_ok>, OP>
               >;
               
template <typename...> struct expr_op_fns_impl;

template <typename T, typename EVAL, typename OP, typename...TERMS>
struct expr_op_fns_impl<T, EVAL, OP, list<TERMS...>>
{
    using type = expr_op_fns_impl;

    static constexpr T value[] = {{hash_fn(TERMS()), EVAL(OP(), TERMS())} ...};
    static constexpr auto size = sizeof...(TERMS);
};

template <typename T, typename EVAL, typename OP, typename ARITY>
using expr_op_fns = _t<expr_op_fns_impl<T, EVAL, OP
                                      , expr_op_terms<OP, ARITY>>>;

}

#endif
