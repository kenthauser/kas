#ifndef PARSER_PARSER_TYPES_H
#define PARSER_PARSER_TYPES_H

#include "expr/expr_types.h"
#include "kas_position.h"
#include "kas_core/ref_loc_t.h"

namespace kas::parser
{
using kas_error_t = core::ref_loc_t<struct kas_diag>;


struct kas_token : kas_position_tagged
{
    operator std::string() const
    {
        return { first, last };
    }
};

// allow streaming of token;
template <typename OS> OS& operator<<(OS&, kas_token const&);

// test if T has `value` member or method
template <typename T, typename = void>
struct has_value : std::false_type {};

template <typename T>
struct has_value<T, std::void_t<decltype(T::value)>>
                 : std::true_type {};

}

namespace kas::expression::detail
{

// NB: in detail namespace, all types are `meta`
template <> struct term_types_v<defn_parser> : list<
                  parser::kas_error_t
                > {};
}

#endif
