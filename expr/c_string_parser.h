#ifndef KAS_EXPR_C_STRING_PARSER_H
#define KAS_EXPR_C_STRING_PARSER_H

// define x3 parser for c-language integer literal
// also support character literal formats such as '\n' and 'ab'

// NB: long & unsigned suffixes are absorbed and ignored.
// NB: unicode literals supported. wchar_t literals use c-local

#include "literal_float.h"
#include "literal_string.h"         // parsed type
#include "int_parser_impl.h"        // support routines to do actual work
#include <stdexcept>
#include <cassert>

namespace kas::expression::literal
{

// create a native X3 parser
// parser returns pointer to e_string instance
template <typename T>
struct c_string_parser : x3::parser<c_string_parser<T>>
{
    using attribute_type = T const *;
    using object_type    = T;

    static bool const has_attribute = true;

    template <typename Iterator, typename Context, typename Attribute>
    bool parse(Iterator& first, Iterator const& last
      , Context const& context, x3::unused_type, Attribute& attr) const
    {
        using char_t = typename Iterator::value_type;

        x3::skip_over(first, last, context);
        Iterator iter(first);     // copy iterator
       
        // look for c-language prefixes (u, u8, U, L)
        auto code = parse_str_pfx(iter, last);

        // done if invalid prefix or not string
        if (*iter++ != '\"')
            return false;
        
        const char *fail{};
        std::basic_string<char_type> str;

        while (*iter != '\"')
        {
            auto [parsed_code, ch] = _str2char(code, iter, last, fail);

            // XXX just char type now...
            str.push_back(ch);
        }

        // update parse location
        first = ++iter;     // skip trailing quote

        auto& value(object_type::add(std::move(str)));

        // convert attribute
        x3::traits::move_to(&value, attr);
        return true;
    }
};

}

#endif

