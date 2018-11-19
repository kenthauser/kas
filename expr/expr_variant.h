#ifndef KAS_EXPR_EXPR_VARIANT_H
#define KAS_EXPR_EXPR_VARIANT_H

/////////////////////////////////////////////////////////////////////
//
// expr_t
//
// expression interface variant
//
// The `expr_t` class is a wrapper over the actual variant. The wrapper
// provides support for the `ref_loc_t` wrapper which is used to wrap
// non-copyable (eg: symbols) and large (eg: core_expr) types into a
// single word. When `visitiing` or `assigning` a variant with a wrapped
// type, the `ref_loc_t` wrapper is automatically added or removed.
//
// The `expr_t` variant is initialzed from meta::list `term_types`.
// The types which are wrapped in `ref_loc_t` should be listed as
// wrapped types. The wrapped types don't need to be defined, just declared.
//
// Facilities provied:
//
// Additional (extended) variant methods are provided by `expr_t`:
//
//  get_p<>() [const]:      return pointer to variant type
//         (extension:)     return pointer to `ref_loc_t` wrapped type
//
//  apply_visitor() [const]; apply visitor to `ref_loc_t` wrapped type, or
//                          variant type if not wrapped
//
//  get_fixed_p() const:    return pointer to fixed value, if present
//  get_loc_p()   const:    return pointer to `kas_loc` value, if available
//
//
// The `expr_t` is based on `boost::variant`, not `std::variant` because
// the `boost::spirit` utilizes the `apply_visitor` and `recursive_variant`
// facilites of the boost implementation. These facilites are not used
// nor needed by the assembler.
//
/////////////////////////////////////////////////////////////////////


#include "expr_types.h"         // base templates for expressions
#include "kas_core/core_types.h"
#include "parser/parser_types.h"
#include "machine_types.h"      // allow CPU & PSEUDO_OPS to override base templates

#include "terminals.h"
#include "kas_core/ref_loc_t.h"

#include <boost/spirit/home/x3/support/ast/variant.hpp>
#include <boost/spirit/home/x3/support/utility/lambda_visitor.hpp>

#include <array>
#include <ostream>

namespace kas::expression::ast
{
namespace x3 = boost::spirit::x3;

namespace detail {
    using namespace meta;
    using namespace meta::placeholders;

    using variant_types = term_types;

    // scan types looking for wrapped types
    template <typename T, typename = void>
    struct is_ref_loc_wrapped: std::false_type {};

    // test for declared (not defined) wrapped types
    template <typename T>
    struct is_ref_loc_wrapped<T, std::void_t<typename T::object_t, typename T::index_t>> :
        std::is_same<T, core::ref_loc_t<typename T::object_t, typename T::index_t>> {};

    struct get_object_t
    {
        template <typename T>
        using invoke = typename T::object_t;
    };

    using plain     = at_c<partition<variant_types, quote<is_ref_loc_wrapped>>, 1>;
    using wrapped   = at_c<partition<variant_types, quote<is_ref_loc_wrapped>>, 0>;
    using unwrapped = transform<wrapped, get_object_t>;

    // XXX cannot reduce below 4* because of std::string...???
    static constexpr std::size_t expr_max_size_t = 8 * sizeof(void*);

    // XXX use std::void_t<>
    template <typename T, typename = void>
    struct has_wrapped_value_type_impl : std::false_type {};

    template <typename T>
    struct has_wrapped_value_type_impl<T, std::void_t<typename T::object_t::value_type>>
                    : std::true_type {};

    template <typename T>
    using has_wrapped_value_type = _t<has_wrapped_value_type_impl<std::decay_t<T>>>;

    // extract wrapped types

    using ref_loc_types     = filter<variant_types, quote<is_ref_loc_wrapped>>;
    using ref_wrapped_types = transform<ref_loc_types, get_object_t>;

    template <typename T>
    using T2Ref = at<ref_loc_types, find_index<ref_wrapped_types, T>>;

    template <typename T, typename List>
    using Ref4T_impl = std::enable_if_t<!in<variant_types, T>::value &&
                                        !std::is_object<T>::value &&
                                        !empty<List>::value
                                      , front<List>
                                      >;

    template <typename T>
    using Ref4T = Ref4T_impl< T
                            , find_if<
                                  ref_wrapped_types
                                , lambda<_a, std::is_constructible<_a, T>>
                                >
                            >;

    // using all_variant_types = term_types;
    using expr_x3_variant = apply<quote<x3::variant>, variant_types>;

    // be sure default variant type is e_fixed_t
    static_assert(std::is_same<
                      front<variant_types>
                    , e_fixed_t
                    >::value
                , "expr_t variant default is not e_fixed_t");

    // helper to find "forwarded" types
    template <typename T>
    static constexpr auto in_variant_v = in<variant_types, T>::value;
    // template <typename T>
    // static constexpr auto is_forwarded_v = in<forwarded_types, std::decay_t<T>>::value;

    template <typename T>
    static constexpr auto is_expr_v        = in<term_types, std::decay_t<T>>::value;

    template <typename T>
    using is_ref_loc_t = in<ref_loc_types, std::decay_t<T>>;


    template <typename T>
    static constexpr auto is_ref_loc_value_v     = has_wrapped_value_type<T>::value;
    // static constexpr auto is_ref_loc_value_v     = false;

    template <typename T>
    static constexpr auto is_ref_loc_v     = is_ref_loc_t<T>::value && !is_ref_loc_value_v<T>;

    template <typename T>
    static constexpr auto is_non_ref_loc_v  = !is_ref_loc_t<T>::value;


    template <typename T>
    static constexpr auto is_ref_wrapped_v = in<ref_wrapped_types, std::decay_t<T>>::value;

    // primary template handles types that have no nested `get_p` member:
    template <typename, typename = e_fixed_t, typename = void>
    struct has_get_template : std::false_type {};

    // specialization recognizes types that do have a nested `get_p` member:
    template <typename T, typename U>
    struct has_get_template<T, U,
        std::void_t<decltype(std::declval<T>().template get_p<U>())>>
            : std::true_type {};
}

struct expr_t : detail::expr_x3_variant
{
    using base_type = detail::expr_x3_variant;
    using base_type::get;
    using base_type::base_type;

    using variant_types   = detail::variant_types;
    using unwrapped_types = meta::concat<detail::plain, detail::unwrapped>;

    expr_t() = default;

    template <typename T, typename = std::enable_if_t<
                    detail::is_expr_v<T>
                 && !std::is_integral_v<std::decay_t<T>>
                 >>
    expr_t(T&& t) : base_type(std::forward<T>(t)) {}

    template <typename T, typename U = std::decay_t<T>, typename = std::enable_if_t<
                            std::is_integral_v<U> 
                            && sizeof(U) <= sizeof(e_fixed_t)>>
    expr_t(T t) : base_type(static_cast<e_fixed_t>(t)) {}

    template <typename T, typename = std::enable_if_t<
                    detail::is_ref_wrapped_v<T>// && !std::is_arithmetic_v<T>
                    >>
    expr_t(T const& t) : expr_t(t.ref()) {}

    // XXX the templated constructor conflicts with integral ? 
    expr_t(long double t) : expr_t(kas_float::add(t)) {}

#if 0
    // hold host integral types too big for `e_fixed_t`
    template <typename T, typename = std::enable_if_t
                <std::is_integral_v<T> && (sizeof(T) > sizeof(e_fixed_t))>>
    expr_t(T t) : expr_t(kas_bigint_host::add(t)) {}
#endif
    
private:
    // method to call visitor with `type` or `unwrapped` type
    // if `unwrapped` type has `value_type`, use actual value
    // (eg. floating points are too big to be put in variant directly)
    template <typename T, typename F>
    static auto do_visitor(T&& e, F&& v)
        -> decltype(e.base_type::apply_visitor(std::declval<F>()))
    {
        // if `ref_loc_t` type, call visitor with `wrapped` object
        using RT = typename std::decay_t<F>::result_type;

        return e.base_type::apply_visitor(
                    x3::make_lambda_visitor<RT>(
            [&v](auto&& node)
                -> std::enable_if_t<detail::is_ref_loc_value_v<decltype(node)>, RT>
                {
                    return v(node.get()());
                },
            [&v](auto&& node)
                -> std::enable_if_t<detail::is_ref_loc_v<decltype(node)>, RT>
                {
                    return v(std::forward<decltype(node)>(node).get());
                },
            [&v](auto&& node)
                -> std::enable_if_t<detail::is_non_ref_loc_v<decltype(node)>, RT>
                {
                    return v(std::forward<decltype(node)>(node));
                }
            ));
    }

    // recursively perform get_p<> on variant node to retrive `const *`
    template <typename T, typename = void>
    T const *get_nested_p() const
    {
        using RT = T const*;

        return apply_visitor(x3::make_lambda_visitor<RT>(
            [](T const& node) -> RT
                {
                    return &node;
                },
            [](auto const& node)
                -> std::enable_if_t<
                            detail::has_get_template<decltype(node)>::value
                          , RT
                          >
                {
                    return node.template get_p<T>();
                },
            [](auto const& node)
                -> std::enable_if_t<
                            !detail::has_get_template<decltype(node)>::value
                          , RT
                          >
                {
                    return {};
                }
            ));
    }

    // true if variant holds an integral type
    template <typename T>
    bool is_integral() const
    {
        return apply_visitor(x3::make_lambda_visitor<bool>(
            [](auto& node) -> std::enable_if_t<
                          std::is_integral_v<decltype(node)>
                        , bool>
                { return true; },
            [](auto& node) -> std::enable_if_t<
                          !std::is_integral_v<decltype(node)>
                        , bool>
                { return false; }
         ));
    }

public:
    // use common routine for const/mutable
    template <typename F>
    auto apply_visitor(F&& v)
    {
        return this->do_visitor(*this, std::forward<F>(v));
    }
    template <typename F>
    auto apply_visitor(F&& v) const
    {
        return this->do_visitor(*this, std::forward<F>(v));
    }

    template <typename T>
    std::enable_if_t<detail::in_variant_v<T>, T const*>
    get_p() const
    {
        return boost::get<T>(&this->get());
    }
    // template <typename T>
    // std::enable_if_t<detail::in_variant_v<T>, T*>
    // get_p()
    // {
    //     return boost::get<T>(&this->get());
    // }

    // check wrapped types
    template <typename T>
    std::enable_if_t<detail::is_ref_wrapped_v<T>, T const*>
    get_p() const
    {
        if (auto p = boost::get<detail::T2Ref<T>>(&this->get()))
            return &p->get();
        return nullptr;
    }

    // template <typename T>
    // std::enable_if_t<detail::in_variant_v<x3::forward_ast<T>>, T*>
    // get_p()
    // {
    //     if (auto p = boost::get<x3::forward_ast<T>>(&this->get()))
    //         return p->get_pointer();
    //     return nullptr;
    // }

    // XXX need to refactor `get_fixed_p` so it can handle integral
    // XXX types larger than `e_fixed_t`. For instance, a `std::size_t`
    // XXX instance of `expr_t` returns nullptr for `get_fixed_p`
    auto get_fixed_p() const { return get_nested_p<e_fixed_t>(); }
    auto get_loc_p()   const { return get_nested_p<kas::parser::kas_loc>() ;  }
    auto is_missing()  const { return false; }
};

static_assert (sizeof(expr_t) == sizeof(detail::expr_x3_variant)
                , "expr_t sizeof error");

// declare ostream function
std::ostream& operator<<(std::ostream&, expr_t const&);
}
#endif
