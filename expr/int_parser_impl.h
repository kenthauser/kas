#ifndef KAS_EXPR_INT_PARSER_IMPL_H
#define KAS_EXPR_INT_PARSER_IMPL_H

// parsing character & string literals
// using c++11/14/17 languange formats

// The c++11 language has five different string formats:
// no prefix = narrow (char), 'L' prefix = wide (compiler-dependent wchar_t)
// and 'u8'/'u'/'U' prefixes indicating UTF-8/16/32 encodings. (UTF-32
// is actually better refered to as UCS-4, as it's not really a
// "transform-code". But UTF-32 seems to be in wide use.)
//
// Similarly c++11/17 provides the same prefixes for character constants,
// with the added bonus of a no-prefix, multi-character constant.
//
// Compounding the problems are the multiple escape sequences:
// \octal, \x, \u & \U which can introduce binary data.
//
// This implementation attempts to rationalize the rules as follows:
//
// All string data is stored as a sequence of 32-bit data in a STL container.
// All container elements are limited to 8-bits (for narrow/utf-8 formats,
// 16-bits (for "wide"/utf-16 formats), or 32-bits for utf-32 format.
//
// NB: narrow characters are not "widened" in conversion to
// 16-bits. `wchar_t` is compiler (implementation defined)
// so there is no "standard" way to go from 8-bit to 16/32-bit.
// NB: this likely makes wide strings in the assembler useless...
//
// C/C++ declare the "\x" escape sequence to absorb an unlimited
// hex character sequence. As it's unclear what to do with the excess,
// I limit "\x" to number of hex charcters that fit in underlying data type.
// NB: This implementation decision to limit \x to (2*sizeof(T)) is primarily
// to facilitate utf-8 data encoded as \x sequences (to prevent stray
// hex characters following the sequence to screw up the string).
//
// "\u" & "\U" escape sequences are limited to the unicode character/string
// types. (otherwise, how does one save the code-point value?)
// NB: exception ASCII code point values (U+0000 -> U+007F) are always A-OK.
//
// "\x" sequences in unicode string & character literals are presumed
// to properly encode unicode values & are passed through unchanged.
// NB: it is not proper for assembler to possibly translate one code-point
// encoding with another -- (consider combining code-points) -- just pass
// it thru.
//
// "u8" character constants must be proper ASCII character (per c++17 standard)
//
// multi-character constants count is limited to sizeof(e_fixed_t<>) chars
// (ie 4 narrow characters for a 32-bit type). narrow & wide string types
// support multi-character.
//
// multi-line strings not allowed: assemblers are line-oriented
//
// Raw strings not implemented
//

// include literal type headers because they hold forward declarations
// for parser routines

#include "literal_string.h"

#include <stdexcept>
#include <cassert>

namespace kas::expression::literal
{

using namespace kas::parser;
using namespace boost::spirit::x3;

// identify character encoding from prefix code
template <typename Iterator>
e_string_code parse_str_pfx(Iterator& it, Iterator const& last)
{
    // permissive parser: allows invalid prefixes such as Uu to pass thru...
    for(auto code = str_code_c; it != last; ++it)
    {
        switch (*it)
        {
            default:
                return str_code_none;       // invalid prefix
            case '"': case '\'':
                return code;                // found quote...
            case 'L':
                code = str_code_w;
                continue;
            case 'U':
                code = str_code_u32;
                continue;
            case 'u':
                code = str_code_u16;
                continue;
            case '8':
                if (code != str_code_u16)
                    return str_code_none;
                code = str_code_u8;
                continue;
        }
    }
    return str_code_none;                   // ran out of sting
}


// convert `Char` to int if ok for RADIX (-1 otherwise)
template <int RADIX = 10, typename Char>
constexpr int _char2int(Char ch)
{
    int result = -1;
    if (ch >= '0' && ch <= '9')
        result = ch - '0';
    else if (ch >= 'a' && ch <= 'z')
        result = ch - 'a' + 10;
    else if (ch >= 'A' && ch <= 'Z')
        result = ch - 'A' + 10;
    return result < RADIX ? result : -1;
}

// convert string to int until not in RADIX or MAX_DIGITS
// SEP_CHAR is c++14 separator charater (if allowed)
template <int RADIX, int SEP_CHAR, typename Iterator>
std::uintmax_t _str2int(Iterator& it, Iterator const& end, const char *& fail
                       , int max_digits, int min_digits)
{
    // largest supported type
    std::uintmax_t result = 0;

    auto decode_str = [&](int& n)
    {
        for ( ; n && it != end; ++it)
        {
            auto ch = *it;
            if (SEP_CHAR && (ch == SEP_CHAR))
                continue;
            auto i = _char2int<RADIX>(ch);
            if (i < 0)
                break;

            result = result * RADIX + i;
            --n;
        }
    };

    max_digits -= min_digits;
    decode_str(min_digits);
    if (min_digits)
        fail = "E literal too short";
    else
        decode_str(max_digits);
    return result;
}

//
// Declare additional types/functions to parse escape sequences,
// unicode characters, and multi-byte sequences
//
// Note: the same routines are used for parsing character literals
// (single quotes) resulting in a integer result and string literals
// (double quotes) resulting in a "string" result.
//

template <typename It>
auto _str2char(e_string_code code, It& it, It const& end, const char *& fail)
                    -> std::pair<e_string_code, uint64_t>
{
    auto parse = [&]() -> uint64_t
        {
            if (it == end)
            {
                fail = "E unterminated literal";
                return {};
            }

            auto ch = *it++;    // consume initial character

            if (ch != '\\')
                return ch;      // i'm just a `char`

            if (it == end)
            {
                fail = "E unterminated literal";
                return {};
            }

            // octal sequences begin immediately after escape
            switch (ch)
            {
                case '0': case '1': case '2': case '3':
                case '4': case '5': case '6': case '7':
                    // c/c++: max length: 3 chars
                    code = str_code_none;       // flag raw value
                    return _str2int<8>(it, end, fail, 3);
                
                default:
                    break;
            }

            ch = *it++;         // get continuation character

            // from: cppreference.com: escape sequences (c/c++)
            // c requires diagnostic for non-enumerated escapes
            switch (ch)
            {
                default:    return ch;
                case '\'':  return '\'';
                case '"':   return '\"';
                case '?':   return '\?';
                case '\\':  return '\\';
                case 'a':   return '\a';
                case 'b':   return '\b';
                case 'f':   return '\f';
                case 'n':   return '\n';
                case 'r':   return '\r';
                case 't':   return '\t';
                case 'v':   return '\v';

                case 'x':
                    // c/c++: unlimited length
                    // c/c++: unspecified result if exceeds value_t
                    code = str_code_none;       // flag raw value
                    return _str2int<16>(it, end, fail);
                case 'u':
                    code = str_code_u16;        // value is unicode-16
                    return _str2int<16>(it, end, fail, 4, 4);
                case 'U':
                    code = str_code_u32;        // value is unicode-32
                    return _str2int<16>(it, end, fail, 8, 8);
            }
        };

    // use lambda to get value
    auto value = parse();
    return { code, value };
}
#if 0
// template <typename T, typename is_unicode_t = std::false_type>
template <typename RT>
struct decode_char_seq
{
    // emulate a `kas_string_t` for `integral` type
    template <typename T = RT>
    struct str_char_t
    {
        void push_back(T ch, uint8_t ch_size = 1)
        {
            max_chars -= ch_size;
            // if (!max_chars--)
            //     throw ....
            if (!value)
                value = ch;
            else
                value = (value << (ch_size * 8)) | ch;
        };

        T       value{};
        uint8_t max_chars  = sizeof(T);
    };

    decode_char_seq(uint8_t ch_size = sizeof(char)
              , uint8_t unicode = false
              , uint8_t is_char = false
              ) : ch_size(ch_size), unicode(unicode), is_char(is_char) {}


    // create "save" overload so body of parser can stay the same.
    // XXX probably need to refactor out the common routine.

    template <typename T, typename U>
    std::enable_if_t<std::is_same<T, RT>::value>
    save_value(T& ref, U* s_value, RT ch_value) const
    {
        ref = ch_value;
    }

    template <typename T, typename U>
    std::enable_if_t<!std::is_same<T, RT>::value>
    save_value(T& ref, U* s_value, RT ch_value) const
    {
        ref = s_value->ref();;
    }

    template <typename Context>
    void operator()(Context const& ctx) const
    {
        RT mask {};
        typename kas_string::object_t *str_pointer {};
        str_char_t<> char_t {};

        auto push_back = [&](RT ch)
            {
                if (str_pointer)
                    str_pointer->push_back(ch);
                else
                    char_t.push_back(ch, ch_size);
            };

        if (!is_char) {
            str_pointer = &kas_string::object_t::add(ch_size, unicode);
            if (ch_size == 1)
                mask = ~0xff;
        } else {
            // utf-8 characters limited to 7-bit ASCII
            // (per c++17: 5.13.3, note 3)
            if (ch_size == 1 && unicode)
                mask = ~0x7f;
            else if (ch_size == 1)
                mask = ~0xff;
            else if (ch_size == 2)
                mask = ~0xffff;
        }

        // begin iter needs to be lvalue
        auto& range = _attr(ctx);
        auto  it    = range.begin();
        auto  end   = range.end();

        while(it != end)
        {
            auto  result        = _str2char(it, end, ch_size);
            using result_enum_t = decltype(result.second);

            switch(result.second)
            {
                default:
                case result_enum_t::CHAR:
                    break;

                case result_enum_t::UTF16:
                case result_enum_t::UTF32:
                    // if (sizeof(T) == 1)
                    //     throw parse_int_err(it, "unicode sequences not allowed for byte strings");
                    // if (!is_unicode_t())
                    //     warning(it, "\\[uU] sequence only valid for unicode types");
                    // FALLSTHRU
                case result_enum_t::DATA:
                    // if (result.first > max)
                    //     warning(it, "character constant value exceeds maximum");
                    break;
            }
            push_back(result.first);
        }

        // compiler chooses proper overload...
        save_value(_val(ctx), str_pointer, char_t.value);
    }

private:
    uint8_t ch_size = 1;
    uint8_t unicode = false;
    uint8_t is_char = false;
};
#endif
}

#endif

