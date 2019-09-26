#ifndef PARSER_PARSER_TYPES_H
#define PARSER_PARSER_TYPES_H

#include "expr/expr_types.h"
#include "kas_position.h"
#include "kas_core/ref_loc_t.h"

#include <boost/spirit/home/x3.hpp>

namespace kas::parser
{
namespace x3 = boost::spirit::x3;


// KAS ITERATOR Type
using iterator_type = std::string::const_iterator;
using char_type     = typename std::iterator_traits<iterator_type>::value_type;

// blank_type matches spaces or tabs, but not newlines
using skipper_t = x3::ascii::blank_type;

// Declare our Error Handlers  (see "parser/error_handler.h")

// KAS extension for multiple files
template <typename Iter> struct error_handler;

// KAS ERROR type
template <typename> struct kas_diag;
using kas_error_t = core::ref_loc_t<kas_diag>;
using kas_diag_t  = typename kas_error_t::object_t;

using kas_position_tagged = kas_position_tagged_t<iterator_type>;

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
