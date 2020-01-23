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

#include <ostream>

namespace kas::expression::ast
{
namespace x3 = boost::spirit::x3;

namespace detail
{
    using namespace meta;
    using namespace meta::placeholders;

    // test for ref-loc-t wrapped types
    template <typename T, typename = void>
    using is_ref_loc_wrapped = std::is_base_of<core::ref_loc_tag, T>;
    
    // alias to get `object_t` from T
    template <typename T>
    using get_object_t = typename T::object_t;

    // Variant types:
    //
    // `variant_types` is list of actual types
    // `plain`         is list of unwrapped types
    // `wrapped`       is list of ref-loc-t wrapped types
    // `unwrapped`     is list of object types `wrapped` in `wrapped`

    using variant_types = term_types;

    using plain     = at_c<partition<variant_types, quote<is_ref_loc_wrapped>>, 1>;
    using wrapped   = at_c<partition<variant_types, quote<is_ref_loc_wrapped>>, 0>;
    using unwrapped = transform<wrapped, quote<get_object_t>>;

    // re-wrap unwrapped type
    template <typename T>
    using wrap = at<wrapped, find_index<unwrapped, T>>;
    
    // simplify `do_visitor` enable-if types
    template <typename LIST, typename T>
    using decay_in = meta::in<LIST, std::decay_t<T>>;

#if 1
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
    using ref_wrapped_types = transform<ref_loc_types, quote<get_object_t>>;

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
#endif

    // primary template handles types that have no nested `get_p` member:
    template <typename, typename = int, typename = void>
    struct has_get_template : std::false_type {};

    // specialization recognizes types that do have a nested `get_p` member:
    template <typename T, typename U>
    struct has_get_template<T, U,
        std::void_t<decltype(std::declval<T>().template get_p<U>())>>
            : std::true_type {};
    
    // create x3::variant base-type
    using expr_x3_variant = apply<quote<x3::variant>, variant_types>;

    // be sure default variant type is e_fixed_t
    static_assert(std::is_same<front<variant_types>, e_fixed_t>::value
                , "expr_t variant default is not e_fixed_t");

}

struct expr_t : detail::expr_x3_variant
{
    using base_t = detail::expr_x3_variant;

    // XXX inherit move operator, etc

    using variant_types   = detail::variant_types;
    using plain           = detail::plain;
    using wrapped         = detail::wrapped;
    using unwrapped       = detail::unwrapped;

    // XXX deprecated
    using unwrapped_types = meta::concat<detail::plain, detail::unwrapped>;

    // define four public ctors
    // 1. default ctor (value = zero)
    // 2. integral ctor: allow only values which fit in `e_fixed_t`
    // 3. wrapped ctor: allow lvalue refs which have `ref-loc-t` wrapper
    // 4. base_type: not integral nor wrapped

    // 1. default ctor
    expr_t() = default;

    // 2. integral types
    // vacuum up all integral types. filter back as appropriate
    // three types of numeric arguments:
    //  1. integers that can be converted to `e_fixed_t`
    //  2. floating point args
    //  3. big integers
    //
    // create "numeric" ctor which filters out floats
    // then forward to private ctor which filters out big-integers

    // 1. integers that can be converted to `e_fixed_t`
    template <typename T, typename = std::enable_if_t<
                                        std::is_integral_v<
                                            std::remove_reference_t<T>
                                     >>>
    expr_t(T&& value) : expr_t(value, true) {}
#if 1
    // 2. floating point types that can be converted to `e_float_t`
    template <typename T
            , typename FLT_T = std::enable_if_t<
                            !std::is_void_v<e_float_t>
                           , typename e_float_t::object_t
                           >
            , typename = std::enable_if_t<
                            std::is_floating_point_v<
                                std::remove_reference_t<T>
                         >>>
    expr_t(T&& value, FLT_T = {}) : expr_t(FLT_T::add(std::forward<T>(value))) {}
#endif

    // 3. wrapped ctor types
    // NB: only accept lvalues as `unwrapped` instances must by permanently allocated
    template <typename T, typename = std::enable_if_t<meta::in<unwrapped, T>::value>>
    expr_t(T const& t) : base_t(t.ref()) {}

    // 4. not integral, not floating point, nor wrapped: forward to base_t
    template <typename T
            , typename U = std::decay_t<T>  // remove const && ref
            , typename   = std::enable_if_t<
                             !meta::in<unwrapped, U>::value &&
                             !std::is_integral_v<U>         &&
                             !std::is_floating_point_v<U>
                            >>
    expr_t(T&& value) : base_t(std::forward<T>(value)) {}

private:
    // private ctor excludes integral types too big for `e_fixed_t`
    expr_t(e_fixed_t value, bool) : base_t(value) {}
    
private:
    // method to call visitor with `type` or `unwrapped` type
    // if `unwrapped` type has `value_type`, use actual value
    // (eg. floating points are too big to be put in variant directly)
    template <typename T, typename F>
    static auto do_visitor(T&& e, F&& v)
        -> decltype(e.base_t::apply_visitor(std::declval<F>()))
    {
        // if `ref_loc_t` type, call visitor with `wrapped` object
        using RT = typename std::decay_t<F>::result_type;

        return e.base_t::apply_visitor(
                    x3::make_lambda_visitor<RT>(
            [&v](auto&& node)
                -> std::enable_if_t<detail::decay_in<wrapped, decltype(node)>::value, RT>
                {
                    return v(std::forward<decltype(node)>(node).get());
                },
            [&v](auto&& node)
                -> std::enable_if_t<detail::decay_in<plain, decltype(node)>::value, RT>
                {
                    return v(std::forward<decltype(node)>(node));
                }
            ));
    }

    // perform get_p<> on variant node to retrive `const *`
    template <typename T, typename = void>
    T const *get_nested_p() const
    {
        using RT = T const*;

        return base_t::apply_visitor(x3::make_lambda_visitor<RT>(
            // shortcut for `get_fixed_p`
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

    // method `get_p` maps to `get_if` for std::variant
    template <typename T>
    constexpr T* get_p()
    {
        // check primary types (including wrapped types)
        if constexpr (meta::in<variant_types, T>::value)
            return boost::get<T>(this);
      
        // check unwrapped types
        if constexpr (meta::in<unwrapped, T>::value)
            if (auto p = boost::get<detail::wrap<T>>(this))
                return &p->get();
        
        // no match
        return {}; 
    }

    // const `get_p`: use mutable method, but return const ptr
    template <typename T>
    constexpr T const *get_p() const
    {
        return const_cast<expr_t&>(*this).get_p<T>();
    }

    // utility methods which operate on data in the variant
    auto get_fixed_p() const
    {
        return get_nested_p<e_fixed_t>();
    }

    auto get_loc_p()   const
    {
        return get_nested_p<kas::parser::kas_loc>();
    }
    
    void set_loc(parser::kas_loc const& loc)
    {
        // perform `set_loc` if node has method.

        // ugly (verbose) declaration, but simple SFINAE usage
        // reference is better match than `const` reference for mutable method
        return base_t::apply_visitor(x3::make_lambda_visitor<void>(
            [&loc](auto& node)
                    -> decltype(
                std::declval<std::remove_reference_t<decltype(node)>>().set_loc(loc)
                               )
            {
                node.set_loc(loc);
            },
            [](auto const& node) {}
            ));
    }

    // empty: holds default type
    bool empty() const
    {
        // true iff expr() holds zero int.
        if (auto p = get_fixed_p())
            return !*p;
        return false;
    }
};

static_assert (sizeof(expr_t) == sizeof(detail::expr_x3_variant)
                , "expr_t sizeof error");

// declare ostream function
std::ostream& operator<<(std::ostream&, expr_t const&);
}
#endif
