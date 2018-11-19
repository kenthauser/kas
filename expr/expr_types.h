#ifndef KAS_EXPR_TYPES_H
#define KAS_EXPR_TYPES_H

#include "kas/defn_utils.h"
#include "parser/parser_config.h"

#include <type_traits>
#include <string>


namespace kas {}
namespace kas::expression
{

// open print namespace used for extensions
namespace print {}

// open detail namespace for configuration types
namespace detail {
    using namespace meta;

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
}

template<typename = void> struct e_fixed   { using type = int32_t; };
template<typename = void> struct e_float   { using type = void;    };
template<typename = void> struct e_string  { using type = void;    };

// Forward declare operator precedence type & default to c-language rules
namespace precedence
{
    struct precedence_c;
}
template<typename = void> struct e_precedence { using type = precedence::precedence_c; };

// Forward declare templates for fixed, floating, and string parsers
// These templates are instantiated with a single expression type argument
// NB can't default "type", because that would instantiate e_* templates
template<typename E_FIXED,  typename = void> struct fixed_p;
template<typename E_FLOAT,  typename = void> struct float_p;
template<typename E_STRING, typename = void> struct string_p;

// Declare expression operator type.
// `expr_op` is used in x3 grammar, so it needs to be
// default-constructable & cheap to copy. Use pimpl idiom.

// declare exprssion variant & namespace
// XXX this should probaby be moved to "expr.h"
namespace ast
{
    struct expr_t;
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
        static bool value(void const *) noexcept { return false; }
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
#endif
