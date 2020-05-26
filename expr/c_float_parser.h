#ifndef KAS_EXPR_C_FLOAT_PARSER_H
#define KAS_EXPR_C_FLOAT_PARSER_H

// define x3 parser for floating point liter

#include "literal_float.h"

#include <stdexcept>
#include <cassert>

namespace kas::expression::literal
{

// create a native X3 parser
template <typename T>
struct c_float_parser : x3::parser<c_float_parser<T>>
{
    using attribute_type = T;
    static bool const has_attribute = true;

    template <typename Iterator, typename Context, typename Attribute_>
    bool parse(Iterator& first, Iterator const& last
      , Context const& context, unused_type, Attribute_& attr) const
    {
        x3::skip_over(first, last, context);
        Iterator it(first);     // copy iterator

        // look for (and consume) sign
        bool neg = *it == '-';
        if (neg || *it == '+')
            ++it;

        // select numeric or character parser based on first character
        T value {};
        bool result;
        if (std::isdigit(*it))
            result = parse_int(context, it, last, value, neg); 
        else if (it != first)
            return false;       // +/- prefix not allowed for char literals
        else
            result = parse_char(context, it, last, value, neg);
        
        if (!result)
            return false;

        traits::move_to(value, attr);
        first = it;             // consume parsed value
        return true;
    }
};

}

#endif

