#ifndef KAS_EXPR_LITERAL_STRING_H
#define KAS_EXPR_LITERAL_STRING_H

// declare assembler STRING literal type

// since both strings & floating point types won't fit in a 32-bit int
// (or even a 64-bit int), allocate each on a private deque & reference
// by index. This helps with variant & speeds copying.

// *** STRING CONTAINER TYPE ***

// XXX obsolete comment??
// c++ delcares type types of parsed strings:
// narrow (default), wide('L'), utc-8/16/32 ('u8'/'u'/'U')
// declare a single type using a std::vector<uchar32_t> to hold characters
// with instance variables to describe the string (8/16/32 unicode/host format)

#include "parser/parser_types.h"
#include "kas_core/ref_loc_t.h"
#include "kas_core/kas_object.h"
#include <boost/range/iterator_range.hpp>
#include <iomanip>

#include <limits>

// declare types of strings supported by literals
namespace kas::expression::literal
{
struct e_string_code
{
    constexpr e_string_code(uint8_t size = 0, bool unicode = false)
        : size(size), unicode(unicode) {}

    constexpr bool operator==(e_string_code const& other) const
        { return size == other.size && unicode == other.unicode; }
    constexpr bool operator!=(e_string_code const& other) const
        { return !(*this == other); }


    uint8_t size    : 1;    // bytes per character
    uint8_t unicode : 1;    // true for unicode
};

static constexpr e_string_code str_code_none{};
static constexpr e_string_code str_code_c   {sizeof(char)       , false};
static constexpr e_string_code str_code_w   {sizeof(wchar_t)    , false};
static constexpr e_string_code str_code_u8  {sizeof(char)       , true };
static constexpr e_string_code str_code_u16 {sizeof(char16_t)   , true };
static constexpr e_string_code str_code_u32 {sizeof(char32_t)   , true };

// forward declare parser support routines in `int_parser_impl.h`

// convert string to int until not in RADIX or MAX_DIGITS
// SEP_CHAR is c++14 separator charater (if allowed)
template <int RADIX, int SEP_CHAR = 0, typename Iterator>
const char * _str2int(Iterator& it, Iterator const& end, std::uint64_t& value
                    , int *exponent_p, int max_digits = -1, int min_digits = 1);

// allow `exponent_p` to be omitted from arg list
template <int RADIX, int SEP_CHAR = 0, typename Iterator>
const char * _str2int(Iterator& it, Iterator const& end, std::uint64_t& value
                    , int max_digits = -1, int min_digits = 1)
{
    return _str2int<RADIX, SEP_CHAR>
                    (it, end, value, nullptr, max_digits, min_digits);
}


// look for c-language string prefix codes
template <typename Iterator>
e_string_code parser_str_pfx(Iterator& it, Iterator const& last);
}


// declare type to hold parsed literal strings
namespace kas::expression::detail
{
namespace x3 = boost::spirit::x3;

// basic kas_string: use `ref` for begin/end
template <typename REF, typename T = void>
struct e_string_tpl : core::kas_object<e_string_tpl<REF, T>, REF>
{
    using base_t      = core::kas_object<e_string_tpl<REF, T>, REF>;
    using emits_value = std::true_type;
    using ref_t       = REF;

    // default stored value based on parsed character type
    using value_t     = std::conditional_t<std::is_void_v<T>
                                         , typename parser::char_type
                                         , T
                                         >;

    using base_t::index;
    using base_t::base_t;       // default ctor

    // two ctors: begin/end pair & string
    template <typename Iterator>
    e_string_tpl(Iterator const& begin, Iterator const& end, parser::kas_loc loc = {})
        : str(begin, end), base_t(loc) {}

    e_string_tpl(std::basic_string<value_t>&& str, parser::kas_loc loc = {}) 
        : str(std::move(str)), base_t(loc) {}

public:
    auto& operator()() const
    {
        return str;
    }

    // named method easier to call with pointer 
    auto& value() const { return (*this)(); }

    auto size() const { return str.end() - str.begin(); }

private:
    std::basic_string<value_t> str;
};

// declare a basic "quoted string" parser.
// Accept only ".*" strings.
// parser returns pointer to e_string instance
template <typename T>
struct quoted_string_p : x3::parser<quoted_string_p<T>>
{
    using attribute_type = T const *;
    using object_type    = T;

    static bool const has_attribute = true;

    template <typename Iterator, typename Context, typename Attribute>
    bool parse(Iterator& first, Iterator const& last
      , Context const& context, x3::unused_type, Attribute& attr) const
    {
        x3::skip_over(first, last, context);
        if (*first != '"')
            return false;

        Iterator iter(first);   // don't move on fail
        for (auto start = ++iter; iter != last; ++iter)
        {
            if (*iter != '"')
                continue;
            
            // save result in `e_string` object
            auto& value(object_type::add(start, iter));
            
            // update parse location
            first = ++iter;     // skip trailing quote

            // convert attribute
            x3::traits::move_to(&value, attr);
            return true;
        }
        return false;           // no trailing quotation mark
    }
};
}
#endif

