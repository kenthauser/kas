#ifndef KAS_EXPR_TERMINALS_H
#define KAS_EXPR_TERMINALS_H

#include "expr_types.h"         // get base type templates
#include "literal_float.h"      // type to hold float
#include "literal_string.h"     // type to hold sting
#include "error_messages_base.h"
#include "utility/reduce.h"
#include "parser/token_defn.h"

// include default parsers
#include "c_int_parser.h"
#include "c_float_parser.h"
#include "c_string_parser.h"

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
    // Define default `int`, `float` & `string` parsers
    template <typename T>
    struct dflt_fixed_p : x3::int_parser<T> {};

    template <typename T>
    struct dflt_float_p : x3::real_parser<T, x3::strict_real_policies<T>> {};
    
    template <typename T>
    struct dflt_string_p : quoted_string_p<T> {};   // see `literal_string.h`

    // define default parsers: quoted on value-type
    template <typename> struct fixed_p      : id<quote<dflt_fixed_p>>  {};
    template <typename> struct float_p      : id<quote<dflt_float_p>>  {};
    template <typename> struct string_p     : id<quote<dflt_string_p>> {};

    template <typename> struct err_msg      : id<error_msg> {};

    // declare defaults for string type configuration
    template <typename> struct string_value : id<parser::char_type> {};

    // resolve templated value types
    using fixed_value_t  = _t<fixed_value <>>;
    using float_value_t  = _t<float_value <>>;
    using string_value_t = _t<string_value<>>;
    
    // create floating point type
    template <typename REF>
    using float_host_tpl  = float_host_t<REF, float_value_t, _t<float_fmt<> >>;
    using e_float_ref     = core::ref_loc_tpl<float_host_tpl>;

    // create string type
    template <typename REF>
    using string_host_tpl = e_string_tpl<REF, string_value_t >;
    using e_string_ref    = core::ref_loc_tpl<string_host_tpl>;
}

// expose public interface to types
// declare aliases to reduce `mpl` noise...
using e_fixed_t   =          detail::fixed_value_t;
using e_float_t   = typename detail::e_float_ref  ::object_t;
using e_string_t  = typename detail::e_string_ref ::object_t;
using err_msg_t   = typename detail::err_msg<>    ::type;

using e_data_t    = typename detail::e_data<>     ::type;
using e_addr_t    = typename detail::e_addr<>     ::type;

namespace detail
{
    // NB: in detail namespace, all types are ``meta``

    // declare "standard" tokens
    using tok_fixed  = parser::token_defn_t<KAS_STRING("E_FIXED")
                                 , e_fixed_t
                                 , invoke<_t<fixed_p<>>, fixed_value_t>>;

    using tok_float  = parser::token_defn_t<KAS_STRING("E_FLOAT")
                                 , e_float_t
                                 , invoke<_t<float_p<>>, float_value_t>>;
                                
    using tok_string = parser::token_defn_t<KAS_STRING("E_STRING")
                                 , e_string_t
                                 , invoke<_t<string_p<>>, e_string_t>>;

    // NB: `tok_missing` doesn't really belong in `expr` because it really means no
    //     expression was parsed. Place it here so that it is visable to all modules
    //     operatoring on expressions. Don't include in `terminal` list because it's not.
    using tok_missing = parser::token_defn_t<KAS_STRING("E_MISSING"), void, x3::eps_parser>;

    // NB: Don't include `tok_missing` in `expr_t` tokens, as `missing` always matches
    using expr_terminals = list<tok_fixed
                              , tok_float
                              , tok_string>;

    // NB: need to remove "void" type & "void" parsers. Thus the `zip` list might be better
    // NB: probably better to make "large" `expr_terminals` type & filter there...
    template<> struct term_types_v  <defn_expr> : meta::list<
                                                      e_fixed_t 
                                                    , e_float_ref
                                                    , e_string_ref
                                                    > {};
    
    template<> struct term_parsers_v<defn_expr> : meta::list<
                              tok_fixed
                            , tok_float
                            , tok_string
                            > {};
    
    // remove `void` types & parsers from lists
    using term_types   = filter<all_defns<term_types_v>
                              , not_fn<quote<std::is_void>>>;
    using term_parsers = filter<all_defns<term_parsers_v>
                              , not_fn<quote<std::is_void>>>;
}

// expose term_types & term_parsers in expression namespace
using detail::term_types;
using detail::term_parsers;
using detail::tok_float;
using detail::tok_string;
using detail::tok_missing;

// find token to hold type T
// default to none
template <typename T, typename = void>
struct token_t : meta::id<void> {};

// use member type `token_t` as value
// NB: `token_defn` instances are meta::traits
template <typename T>
struct token_t<T, std::void_t<typename T::token_t>> : T::token_t {};

// specialize for native c-language types
template <typename T>
struct token_t<T, std::enable_if_t<std::is_integral_v<T>>>
                    : detail::tok_fixed {};
template <typename T>
struct token_t<T, std::enable_if_t<std::is_floating_point_v<T>>>
                    : detail::tok_float {};

// specialize for expression defined types
//template <> struct token_t<e_float_t>  : detail::tok_float {};
//template <> struct token_t<e_string_t> : detail::tok_string {};

}

namespace kas
{
    // expose common types in top-level namespace
    using expression::e_fixed_t;
    using expression::e_float_t;
    using expression::e_string_t;
    using expression::err_msg_t;
    using e_diag_t  = parser::kas_diag_t;
}


#endif
