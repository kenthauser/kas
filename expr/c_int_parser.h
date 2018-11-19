#ifndef KAS_EXPR_C_INT_PARSER_H
#define KAS_EXPR_C_INT_PARSER_H

// parsing character & string literals
// using c++11/14/17 languange formats

// <EDITORIAL> I can only speak English in my native ASCII, so every
// string I write can be expressed as a simple string of 7-bit
// characters which map one-to-one to a string of bytes (I don't use emoji).
// It is hard for me to intuit what is supposed to happen
// when someone who happens to have an alpha & omega (or even an 8-bit
// latin-1 character) on their keyboard feels compelled to place
// such between quotations & feed the result to the assembler.
// </EDITORIAL>

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


#include "literal_types.h"
#include <stdexcept>
#include <cassert>

namespace kas::expression::parser
{
    using namespace x3;

    // parsing c++14 integers. define separator character
    static constexpr auto cpp14_sep = '\'';

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

    template <typename Iterator>
    struct parse_int_err : std::runtime_error
    {
        parse_int_err(Iterator where, std::string const& which)
            : runtime_error("expression::parser::parse_int_err")
            , where_(where), which_(which)
            {}

        auto  which() const { return which_; }
        auto& where() const { return where_; }

    private:
        Iterator where_;
        std::string which_;
    };

    // convert string to int until not in RADIX or MAX_DIGITS
    // SEP_CHAR is c++14 separator charater (if allowed)
    template <int RADIX, int SEP_CHAR = 0, typename It>
    constexpr auto _str2int(It& it, It const& end, int max_digits = -1, int min_digits = 1)
    {
        e_fixed_t result = 0;

        auto decode_str = [&](int& n) {
            for ( ; n && it != end; ++it) {
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
            throw parse_int_err<It>(it, "sequence too short");

        decode_str(max_digits);
        return result;
    }

    template <int RADIX>
    auto decode_int = [](auto& ctx)
    {
        auto& range = _attr(ctx);
        auto  it = range.begin();
        _val(ctx) = _str2int<RADIX, cpp14_sep>(it, range.end());
    };

    //
    // Declare additional types/functions to parse escape sequences,
    // unicode characters, and multi-byte sequences
    //
    // Note: same routines are used for parsing character literals
    // (single quotes) resulting in a integer result and string literals
    // (double quotes) resulting in a "string" result.
    //

    template <typename It>
    auto _str2char(It& it, It const& end, int ch_size = 1)
    {
        enum str2char_v { CHAR, DATA, UTF16, UTF32 };
        struct str2char_t : std::pair<uint32_t, str2char_v>
        {
            using base_t = std::pair<uint32_t, str2char_v>;
            using enum_t = str2char_v;

            str2char_t(uint32_t n, str2char_v e = CHAR) : base_t(n, e) {};
        };

        // declare return type (a std::pair)
        auto parse = [&]() -> str2char_t
            {
                assert(it != end);
                auto ch = *it++;    // consume initial character

                if (ch != '\\')
                    return ch;      // i'm just a `char`

                if (it == end)
                    // throw "quoted string ends with escape character"
                    return { 0 };

                ch = *it++;         // get continuation character

                // second of quoted pair
                // from: cppreference.com: escape sequences (c/c++)
                // c requires diagnostic for non-enumerated escapes
                switch (ch) {
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

                    case '0': case '1': case '2': case '3':
                    case '4': case '5': case '6': case '7':
                        // c/c++: max length: 3 chars
                        {
                            --it;       // skip back to rescan first char
                            auto o_value = _str2int<8>(it, end, 3);
                            // if (o_value < 0x7f)
                            //     return o_value;     // ASCII
                            // // if (o_value > std::numeric_limts<UT>.max)
                            // //     raise ///
                            // if (o_value > std::numeric_limits<T>{}.max)
                            //     return { o_value, DATA};
                            return o_value;
                        }
                    case 'x':
                        // c/c++: no limit: unlimited length
                        // c/c++: unspecified result if exceeds value_t
                        // NB: implemenation limit: 2*sizeof(T)
                        return _str2int<16>(it, end, ch_size * 2);
                    case 'u':
                    // XXX not quite right...
                    // need to return value/type pair
                        return static_cast<char16_t>(_str2int<16>(it, end, 4, 4));
                    case 'U':
                        return static_cast<char32_t>(_str2int<16>(it, end, 8, 8));
                }
                return -1;
            };

        // use lambda to get return type as a pair...
        return parse();
    }

    // template <typename T, typename is_unicode_t = std::false_type>
    struct decode_char_seq
    {
        // emulate a `kas_string_t` for `integral` type
        template <typename T = e_fixed_t>
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
        std::enable_if_t<std::is_same<T, e_fixed_t>::value>
        save_value(T& ref, U* s_value, e_fixed_t ch_value) const
        {
            ref = ch_value;
        }

        template <typename T, typename U>
        std::enable_if_t<!std::is_same<T, e_fixed_t>::value>
        save_value(T& ref, U* s_value, e_fixed_t ch_value) const
        {
            ref = s_value->ref();;
        }

        template <typename Context>
        void operator()(Context const& ctx) const
        {
            e_fixed_t mask {};
            kas_string::object_t *str_pointer {};
            str_char_t<> char_t {};

            auto push_back = [&](e_fixed_t ch)
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

    auto parse_quoted(char quote)
    {
        return [quote=quote](auto&& prefix, auto&& decode)
            {
                // XXX improve "raw" pattern to allow parsing "escape quote"
                return (eps >> prefix >> quote)
                    > raw[*(char_ - quote)][decode]
                    > quote;
            };
    }

    // instantiate `parse_quoted` for single & double quotes
    auto parse_char = parse_quoted('\'');
    auto parse_str  = parse_quoted('\"');

    // parse & ignore c/c++ suffixes. In assembler, everything's an int.
    auto parse_int = [](auto&& prefix, auto&& digit_chars, auto&& decode)
    {
        // not no_case because 'LL' / 'll' suffix cannot be 'Ll' nor 'lL'
        auto const suffix_u = lit('U') | lit ('u');
        auto const suffix_l = lit('L') | lit ('l') | lit("LL") | lit("ll");
        auto const suffix = (suffix_u  >> -suffix_l) |
                            (-suffix_l >> -suffix_u);

        return no_case[eps >> prefix]
            >> raw[omit[+digit_chars] % lit(cpp14_sep)][decode]
            >> suffix;
    };

    e_fixed_parser<e_fixed_t> c_fixed_p = "fixed parser (c-rules)";

    // not `no_case` because 'LL' (or 'll') suffix cannot be 'Ll' nor 'lL'
    auto const c_fixed_p_def = lexeme[
        // handle hex(0x*) binary(0b*) octal(0*) & decimal(*) formats...
          parse_int("0x", xdigit,       decode_int<16>)
        | parse_int("0b", char_("01"),  decode_int< 2>)
        | parse_int("0",  char_("0-7"), decode_int< 8>)
        | parse_int(eps,  digit,        decode_int<10>)

        // these parsers can "throw" for invalid unicode sequences
        | parse_char(eps,       decode_char_seq{1, false, true})
        | parse_char(lit("L"),  decode_char_seq{2, false, true})
        | parse_char(lit("u8"), decode_char_seq{1, true , true})
        | parse_char(lit("u"),  decode_char_seq{2, true , true})
        | parse_char(lit("U"),  decode_char_seq{4, true , true})
        ];


    quoted_string_parser<kas_string> c_string_p = "string parser (c-rules)";

    auto const c_string_p_def = lexeme[
        // these parsers can "throw" for invalid unicode sequences
          parse_str(eps,       decode_char_seq{1, false})
        | parse_str(lit("L"),  decode_char_seq{2, false})
        | parse_str(lit("u8"), decode_char_seq{1, true })
        | parse_str(lit("u"),  decode_char_seq{2, true })
        | parse_str(lit("U"),  decode_char_seq{4, true })
        ];

    BOOST_SPIRIT_DEFINE(c_fixed_p)
    BOOST_SPIRIT_DEFINE(c_string_p)
}


#endif
