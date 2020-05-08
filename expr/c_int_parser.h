#ifndef KAS_EXPR_C_INT_PARSER_H
#define KAS_EXPR_C_INT_PARSER_H

// define x3 parser for c-language integer literal
// also support character literal formats such as '\n' and 'ab'

// NB: long & unsigned suffixes are absorbed and ignored.
// NB: unicode literals supported. wchar_t literals use c-local

#include "literal_float.h"
#include "literal_string.h"
#include "int_parser_impl.h"        // support routines to do actual work
#include <stdexcept>
#include <cassert>

namespace kas::expression::literal
{

// create a native X3 parser
template <typename T>
struct c_int_parser : x3::parser<c_int_parser<T>>
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
        else
            result = parse_char(context, it, last, value, neg);
        
        if (!result)
            return false;

        traits::move_to(value, attr);
        first = it;             // consume parsed value
        return true;
    }
    
    // parse numeric sequence
    template <typename Context, typename Iterator>
    bool parse_int(Context const& context, Iterator& it, Iterator const& last
                  , T& value, bool neg) const
    {
        // parsing c++14 integers. define separator character
        static constexpr auto cpp14_sep = '\'';

        // look for c-language prefix (ie: 0x, 0b, 0)
        const char *fail{};
        if (*it != '0')
            value = _str2int<10, cpp14_sep>(it, last, fail);
        else
            switch (*++it)
            {
                case 'x': case 'X':
                    value = _str2int<16, cpp14_sep>(++it, last, fail);
                    break;
                case 'b': case 'B':
                    value = _str2int<2, cpp14_sep>(++it, last, fail);
                    break;
                case '0': case '1': case '2': case '3':
                case '4': case '5': case '6': case '7':
                // case cpp_sep:    // don't think separator can follow octal prefix
                    value = _str2int<8, cpp14_sep>(it, last, fail);
                    break;
                case '8': case '9':
                    return false;   // invalid octal value
                default:
                    value  = {};
                    break;
            }

        // if parse failed, don't need to check suffix
        if (fail)
            return false;
        
        // look for and consume suffixes
        // allow `U`,`u`,`L`,`l`,`LL`,or `ll` in any order
        // fail if unsigned or long suffixes repeated
        bool found_u{}, found_l{};
        for (;;)
        {
            switch (*it)
            {
                case 'u': case 'U':
                    if (found_u)
                        return false;   // only once
                    found_u = true;
                    ++it;
                    continue;

                // disallow lL and Ll
                case 'l':
                    if (found_l)
                        return false;   // only once
                    if (*++it == 'L')
                        return false;   // invalid suffix
                    else if (*it == 'l')
                        ++it;
                    found_l = true;
                    continue;
                case 'L':
                    if (found_l)
                        return false;   // only once
                    if (*++it == 'l')
                        return false;   // invalid suffix
                    else if (*it == 'L')
                        ++it;
                    found_l = true;
                    continue;
                default:
                    break;
            }
            break;
        }

        // XXX should check if max value for T
        if (neg)
            value = -value;

        return true;
    }
    
    // parse character sequence
    template <typename Context, typename Iterator>
    bool parse_char(Context const& context, Iterator& it, Iterator const& last
                    , T& value, bool neg) const
    {

        // define push-back function. multiple character strings are
        // stored big-endian
        auto push_back = [&value](uint64_t ch, unsigned bytes = 1) mutable
            {
                auto room = sizeof(T);
                auto do_push = [&value,&room]
                            (uint64_t ch, unsigned bytes, auto const& do_push) -> bool
                    {
                        // recurse to insert big-endian
                        if (--bytes)
                            if (!do_push(ch >> 8, bytes, do_push))
                                return false;

                        // validate room
                        if (--room < 0)
                            return false;       // too many characters failed
                        value <<= 8;
                        value |= ch;
                        return true;
                    };
                return do_push(ch, bytes, do_push);
            };

        // look for character code prefix
        auto code = parse_str_pfx(it, last);

        // literal characters begin with single quote
        if (*it++ != '\'')
            return false;

        auto bytes = sizeof(T);     // how many "bytes" value holds

        const char *fail {};
        while (*it != '\'')
        {
            auto [parsed_code, ch] = _str2char(code, it, last, fail);
            
            if (fail)
                return false;

            // XXX ignore wchar_t/unicode conversion issues
            // XXX ignore overflow issues...
        };

#if 0

        bool fail{};
        if (*it != '0')
            value = _str2int<10, cpp14_sep>(it, last, fail);
        else
            switch (*++it)
            {
                case 'x': case 'X':
                    value = _str2int<16, cpp14_sep>(++it, last, fail);
                    break;
                case 'b': case 'B':
                    value = _str2int<2, cpp14_sep>(++it, last, fail);
                    break;
                case '0': case '1': case '2': case '3':
                case '4': case '5': case '6': case '7':
                // case cpp_sep:    // don't think separator can follow octal prefix
                    value = _str2int<8, cpp14_sep>(it, last, fail);
                    break;
                case '8': case '9':
                    return false;   // invalid octal value
                default:
                    value  = {};
                    break;
            }

        // if parse failed, don't need to check suffix
        if (fail)
            return false;
#endif
#if 0
    // these parsers can "throw" for invalid unicode sequences
    | parse_char(eps,       decode_char_seq<T>{1, false, true})
    | parse_char(lit("L"),  decode_char_seq<T>{2, false, true})
    | parse_char(lit("u8"), decode_char_seq<T>{1, true , true})
    | parse_char(lit("u"),  decode_char_seq<T>{2, true , true})
    | parse_char(lit("U"),  decode_char_seq<T>{4, true , true})
#endif
        return false;
    }

};

}

#endif

