#ifndef KAS_EXPR_TERMINALS_H
#define KAS_EXPR_TERMINALS_H

#include "expr_types.h"         // get base type templates
#include "literal_types.h"      // get "literal" types (float/string/big-int)
#include "error_messages_base.h"
#include "utility/reduce.h"
#include "parser/token_defn.h"

#include <boost/spirit/home/x3.hpp>
#include <boost/spirit/home/x3/numeric.hpp>
#include <boost/spirit/home/x3/string.hpp>

namespace kas::expression
{
namespace x3 = boost::spirit::x3;
namespace mpl = boost::mpl;

// Declare default "parsers" for fixed/float/string types
// Declare base type value & parser configuration hooks 

namespace detail 
{
    // Default `int` parser
    template <typename T>
    struct dflt_fixed_p : x3::int_parser<T> {};

    // Define default `float` & `string` parsers
    // x3 strict parser requires decimal point
    template <typename T>
    struct dflt_float_p : x3::real_parser<T, x3::strict_real_policies<T>> {};

    template <typename T>
    using dflt_string_p = x3::rule<struct _default_qs, T>;
    
    // define default parsers: quoted on value-type
    template <typename> struct fixed_p      : id<quote<dflt_fixed_p>>  {};
    template <typename> struct float_p      : id<quote<dflt_float_p>>  {};
    template <typename> struct string_p     : id<quote<dflt_string_p>> {};

    template <typename> struct err_msg      : meta::id<error_msg> {};

    // declare defaults for floating point type configuration
    template <typename> struct float_value  : id<long double> {};
    template <typename> struct float_fmt    : id<struct ieee754> {};

    // declare defaults for string type configuration
    template <typename> struct string_value : id<parser::char_type> {};
    
    // resolve types & create floating point type
    using float_value_t = _t<float_value<>>;
    using float_fmt_t   = _t<float_fmt<>>;
    //using float_host_t  = _t<bind_back<quote<float_host_ieee>, float_value_t, float_fmt_t>>;
    template <typename REF>
    using float_host_t  = float_host_ieee<REF, float_value_t, float_fmt_t>;
    using kas_float     = _t<core::ref_loc_t<float_host_t>>;

    // set type default values
    template <typename> struct e_float  : meta::id<kas_float> {};
    template <typename> struct e_string : meta::id<kas_string> {};
}
// declare aliases to reduce `mpl` noise...
using e_fixed_t   = typename detail::e_fixed<>  ::type;
using e_float_t   = typename detail::e_float<>  ::type;
using e_string_t  = typename detail::e_string<> ::type;
using err_msg_t   = typename detail::err_msg<>  ::type;

using e_data_t    = typename detail::e_data<>   ::type;
using e_addr_t    = typename detail::e_addr<>   ::type;

//using e_bigint_host_t  = kas_bigint_host;
//using e_float_fmt = typename float_fmt<>::type;

#if 1
namespace detail
{
    // NB: in detail namespace, all types are ``meta``

    // declare "standard" tokens
    using tok_fixed  = parser::token_defn_t<KAS_STRING("E_FIXED")
                                 , e_fixed_t
                                 , meta::invoke<fixed_p<>::type, e_fixed_t>>;
    using tok_float  = parser::token_defn_t<KAS_STRING("E_FLOAT")
                                 , e_float_t
                                 , meta::invoke<detail::float_p<>::type, e_float_t>>;
    using tok_string = parser::token_defn_t<KAS_STRING("E_STRING")
                                 , e_float_t
                                 , meta::invoke<detail::string_p<>::type, e_string_t>>;

using expr_terminals = list<tok_fixed, tok_float, tok_string>;

#if 0
    // zip expr terminals into lists of types and parsers
    using zip_term_types = zip<expr_terminals>;
    template<> struct term_types_v<defn_expr>   : at_c<zip_term_types, 0> {};
    template<> struct term_parsers_v<defn_expr> : at_c<zip_term_types, 1> {};
#else
    // NB: need to remove "void" type & "void" parsers. Thus the `zip` list might be better
    // NB: probably better to make "large" `expr_terminals` type & filter there...
    template<> struct term_types_v  <defn_expr> : meta::list<e_fixed_t, e_float_t, e_string_t> {};
    //template<> struct term_parsers_v<defn_expr> : meta::list<tok_fixed, tok_float, tok_string> {};
    //template<> struct term_parsers_v<defn_expr> : meta::list<tok_fixed, tok_float> {};
    //template<> struct term_parsers_v<defn_expr> : meta::list<tok_fixed, tok_string> {};
    template<> struct term_parsers_v<defn_expr> : meta::list<tok_fixed, tok_fixed> {};
    //template<> struct term_parsers_v<defn_expr> : meta::list<x3::int_parser<e_fixed_t>> {};
#endif
    // remove `void` types & parsers from lists
    using term_types   = filter<all_defns<term_types_v>,   not_fn<quote<std::is_void>>>;
    using term_parsers = filter<all_defns<term_parsers_v>, not_fn<quote<std::is_void>>>;
}

// expose term_types & term_parsers in expression namespace
using detail::term_types;
using detail::term_parsers;
using tok_fixed_t = meta::at_c<term_parsers, 0>;

// find token to hold type T
// default to none
template <typename T, typename = void>
struct token_t : meta::id<void> {};

// use member type `token_t` as value
template <typename T>
struct token_t<T, std::void_t<typename T::token_t>> : T::token_t {};

// specialize default types
template <> struct token_t<e_fixed_t> : detail::tok_fixed {};
}
#endif


namespace kas
{
    // expose common types in top-level namespace
    using expression::e_fixed_t;
    using expression::e_float_t;
    using expression::e_string_t;
    using expression::err_msg_t;
    using e_diag_t  = parser::kas_diag_t;
    //using expression::e_string_t;
}

#endif
