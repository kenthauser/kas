#ifndef KAS_EXPR_TERMINALS_H
#define KAS_EXPR_TERMINALS_H

#include "expr_types.h"         // get base type templates
#include "literal_types.h"
#include "format_ieee754.h"

#include "machine_types.h"      // get overrides
#include "utility/reduce.h"

#include <boost/spirit/home/x3.hpp>
#include <boost/spirit/home/x3/numeric.hpp>
#include <boost/spirit/home/x3/string.hpp>


namespace kas::expression
{
namespace x3 = boost::spirit::x3;
namespace mpl = boost::mpl;

namespace parser
{
    template <typename T>
    using quoted_string_parser = x3::rule<struct _qs, T>;

    template <typename T>
    using e_fixed_parser = x3::rule<struct _fixed_p, T>;

    // x3 strict parser requires decimal point
    template <typename T, typename = void>
    struct strict_float_p : x3::real_parser<T, x3::strict_real_policies<T>> {};

    template <typename T>
    using e_float_parser = x3::rule<struct _float_p, T>;
}

// declare default fixed/float
template <typename T, typename>
struct fixed_p : parser::e_fixed_parser<T> {};

template <typename T, typename>
struct float_p : parser::e_float_parser<T> {};

// declare default floating base & formatter
// delete formatter if no floating point
template <typename> struct float_base : meta::id<detail::float_host_t> {};
template <typename> struct float_fmt  : meta::id<ieee754> {};
//template <> struct float_fmt<void>    : meta::id<void> {};
//template <> struct float_p<void>      : meta::id<void> {};

// default floating point reference type. Allow base type to be deleted.
template <typename>
struct e_float
{
    using base = meta::_t<float_base<>>;
    using type = std::conditional_t<!std::is_void_v<base>
                                  , core::ref_loc_t<base>
                                  , void
                                  >;
};

// default quoted_string parser ignores escape sequences
template <typename T, typename>
struct string_p : parser::quoted_string_parser<T> {};

// default string reference type
template <> struct string_p<void> : meta::id<void> {};

template <typename>
struct e_string
{
    using type = core::ref_loc_t<detail::kas_string_t>;   
};

// declare aliases to reduce `mpl` noise...
using e_fixed_t   = typename e_fixed<>::type;
using e_float_t   = typename e_float<>::type;
using e_string_t  = typename e_string<>::type;
//using e_bigint_host_t  = kas_bigint_host;
using e_float_fmt = typename float_fmt<>::type;

namespace detail
{
    // NB: in detail namespace, all types are ``meta``
    using namespace meta;

    using expr_terminals = list<
          list<e_fixed_t,   fixed_p<e_fixed_t>>
        , list<e_float_t,   float_p<e_float_t>>
        , list<e_string_t,  string_p<e_string_t>>
//        , list<e_bigint_host_t,  void>
        >;

    // zip expr terminals into lists of types and parsers
    using zip_term_types = zip<expr_terminals>;
    template<> struct term_types_v<defn_expr>   : at_c<zip_term_types, 0> {};
    template<> struct term_parsers_v<defn_expr> : at_c<zip_term_types, 1> {};

    // remove `void` types & parsers from lists
    using term_types   = filter<all_defns<term_types_v>,   not_fn<quote<std::is_void>>>;
    using term_parsers = filter<all_defns<term_parsers_v>, not_fn<quote<std::is_void>>>;

    auto inline term_op_p()
    {
        // test parsers in reverse order (eg: try `dot` before `float` before `fixed`)
        using parser_tuple = apply<quote<std::tuple>, reverse<term_parsers>>;
        return kas::reduce_tuple(std::bit_or<>{}, parser_tuple{});
    }
}

// expose term_types & term_op_p in expression namespace
using detail::term_types;
using detail::term_op_p;
}

// specialize std::arithmetic<> for `kas_float`
// NB: perfectly legal to add full specializations to namespace `std`

namespace std
{
// template <> is_arithemetic<float_host_t> : std::true_type {};
}

#endif
