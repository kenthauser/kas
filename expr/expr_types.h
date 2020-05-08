#ifndef KAS_EXPR_TYPES_H
#define KAS_EXPR_TYPES_H

// `defn_utils` holds names of specialized types for `term_types_v` et. al.
#include "kas/defn_utils.h"

#include <meta/meta.hpp>
#include <type_traits>


namespace kas {}        // declare namespace
namespace kas::expression
{
// forward declare various types

// open print namespace used for `print` extensions
namespace print {}

// Forward declare operator precedence type & default to c-language rules
namespace precedence
{
    struct precedence_c;
}

// open detail namespace for configuration types
namespace detail
{
    using namespace meta;

    // *** Declare expression variant types & operations configuration hooks ****

    // Declare expression terminal element definition:
    // Separate defns for types and parsers
    // NB: types must be complete to be added to `boost::variant`
    // NB: parsers can be linked in from modules

    template <typename = void> struct term_types_v   : list<> {};
    template <typename = void> struct term_parsers_v : list<> {};

    // Declare expression binary and unary operation definitions
    // see bop_traits/pfx_traits/etc in `operators.h` for format
    template <typename = void> struct bin_ops_v : list<> {};
    template <typename = void> struct pfx_ops_v : list<> {};
    template <typename = void> struct sfx_ops_v : list<> {};

    // accumulate program options
    template <typename = void> struct options_types_v : list<> {};

    // *** Declare base type value, parser, and format hooks ***

    // NB: can set `fixed` default because no include header issue
    template <typename = void> struct e_fixed : meta::id<int32_t> {};

    // Forward declare templates for floating point configuration
    template<typename = void> struct float_value;
    template<typename = void> struct float_fmt;
    
    // Forward declare template for string character type
    template<typename = void> struct string_value;

    // Forward declare templates for fixed, floating, and string parsers
    // These templates are instantiated with a single expression type argument
    // NB can't default "type", because that would instantiate e_* templates
    template<typename = void> struct fixed_p;
    template<typename = void> struct float_p;
    template<typename = void> struct string_p;
    
    // declare info about processor targeted by assembler
    template<typename = void> struct e_data : meta::id< int32_t> {};
    template<typename = void> struct e_addr : meta::id<uint32_t> {};
    template<typename = void> struct err_msg;

    // *** Declare operator precidence configuration hooks ****

    template<typename = void> struct e_precedence : id<precedence::precedence_c> {};
}

//
// Support type alias for `expr_fits`: emits_value() if data is emitted
//

// declare result enum used to evaluate enum values
enum fits_result { NO_FIT = 0, MIGHT_FIT, DOES_FIT };

// type alias to determine if expression emits value
// base template: default false
template <typename, typename = void>
struct emits_value : std::false_type {};

// first partial specialization: resolve as `emits_value` member type
template <typename T>
struct emits_value<T, std::void_t<typename T::emits_value>>
        : T::emits_value {};

// second partial specialization: true if arithmetic type
template <typename T>
struct emits_value<T, std::enable_if_t<std::is_arithmetic<T>::value>>
        : std::true_type {};


// Template the `zero` in divide-by-zero test
// always evaluated with "plain" types

namespace detail
{
    struct is_not_zero
    {
        static constexpr bool value(void const *) noexcept { return false; }
    };
}

// default is not zero
// NB: using non-tempalted base class reduces code bloat
template <typename T, typename = void>
struct is_zero : detail::is_not_zero {}; 

// specialize for arithmetic types
template <typename T>
struct is_zero<T, std::enable_if_t<std::is_arithmetic_v<T>>>
{
    static bool value(void const *v) noexcept
    { 
        return *static_cast<T const *>(v) == 0;
    }
};

// specialize to use member-type `is_zero` if defined
template <typename T>
struct is_zero<T, std::void_t<typename T::is_zero>> : T::is_zero {};

}

// declare exprssion variant & namespace
namespace kas::expression::ast
{
    struct expr_t;
}

// expose expr_t to global namespace
namespace kas
{
    using expression::ast::expr_t;
}

#endif
