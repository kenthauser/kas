#ifndef KAS_EXPR_LITERAL_STRING_H
#define KAS_EXPR_LITERAL_STRING_H

// declare assembler STRING literal type

// since both strings & floating point types won't fit in a 32-bit int
// (or even a 64-bit int), allocate each on a private deque & reference
// by index. This helps with variant & speeds copying.

// *** STRING CONTAINTER TYPE ***

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

namespace kas::expression::detail
{
namespace x3 = boost::spirit::x3;

// basic kas_string: use `ref` for begin/end
template <typename REF>
struct kas_string_obj : core::kas_object<kas_string_obj<REF>, REF>
{
    using base_t      = core::kas_object<kas_string_obj<REF>, REF>;
    using emits_value = std::true_type;
    using ref_t       = REF;

    using base_t::index;
    using base_t::base_t;       // default ctor

    // should really store begin/end pair
    kas_string_obj(std::string const& str, parser::kas_loc loc = {}) 
        : str(str), base_t(loc) {}

public:
    auto& operator()() const
    {
        return str;
    }

    // named method easier to call with pointer 
    auto value() const { return (*this)(); }

private:
    std::string str;
};


// parser returns pointer to string reference
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
        using char_t = typename Iterator::value_type;

        x3::skip_over(first, last, context);
        if (*first != '"')
            return false;

        auto iter = first;      // don't move on fail
        for (auto start = ++iter; iter != last; ++iter)
        {
            if (*iter != '"')
                continue;
            
            // get result
            std::string str(&*start, &*iter);
            auto& value(object_type::add(str));
            
            // update parse location
            first = ++iter; // skip trailing quote
            // convert attribute
            x3::traits::move_to(&value, attr);
            return true;
        }
        return false;       // no trailing quotation mark
    }
};
}
#endif

