#ifndef KAS_EXPR_LITERAL_TYPES_IMPL_H
#define KAS_EXPR_LITERAL_TYPES_IMPL_H

// declare assembler FLOATING & STRING literal types

// since both strings & floating point types won't fit in a 32-bit int
// (or even a 64-bit int), allocate each on a private deque & reference
// by index. This helps with variant & speeds copying.

// *** FLOATING POINT TYPES ***

// declare 2 types for literal floating point numbers:
// un-interpreted (character iterator range) & host (typically long double)

// un-interpreted is typically held until value is emitted & is then
// directly converted to emitted binary. `host` format is used when
// calculation is performed on floating point value (unusual)

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
#if 0
template <typename REF, typename VALUE>>
struct kas_string_t : core::kas_object<kas_string_t<REF, VALUE>, REF>
{
    using base_t      = core::kas_object<kas_string_t<REF, VALUE>, REF>;
    using emits_value = std::true_type;
    using ref_t       = REF;

    using base_t::index;
        
    // doesn't participate in expression evaluation
    using not_expression_type = void;


    // access ctor thru static `base_t::add` method
    kas_string_t(uint8_t ch_size = 1, uint8_t unicode = false)
        : ch_size_(ch_size), unicode_(unicode) {}

    kas_string_t(const char *p) : kas_string_t()
    {
        while (*p)
            data.push_back(*p++);
    }

private:
    // create utf-8 std::string
    // XXX move to support .cc file
    std::string& get_str() const
    {
        if (c_str_.empty()) {
            std::ostringstream os;
            os << std::hex << "\"";

            for (auto ch : data) {
                // if control-character: display as hex
                if (ch <= 0x1f)
                    os << std::setw(2) << ch;
                else if (ch < 0x7f)
                    os << static_cast<char>(ch);
                else if (is_unicode() && ch <= 0xffff)
                    os << "U+" << std::setw(4) << ch << " ";
                else if (is_unicode() && ch_size_ > 1)
                    os << "U+" << std::setw(6) << ch << " ";
                else
                    os << "\\x" << std::setw(ch_size_ * 2);
            }
            os << "\"";
            c_str_ = os.str();
        }
        return c_str_;
    }

public:
    // string manipulation methods
    auto  begin()      const { return data.begin(); }
    auto  end()        const { return data.end();   }
    auto  size()       const { return data.size();  }
    auto  ch_size()    const { return ch_size_;     }
    bool  is_unicode() const { return unicode_;     }
    void  push_back(uint32_t ch) { data.push_back(ch); }
    auto  c_str() const { return get_str().c_str();   }

    // template <typename OS>
    // friend OS& operator<<(OS& os, kas_string_t const& str)
    //     { return os << str.get_str(); }
    template <typename OS>
    void print(OS& os) const
        { os << get_str(); }

private:
    std::vector<uint32_t> data;
    mutable std::string c_str_;
    uint8_t ch_size_;       // sizeof(T)
    uint8_t unicode_;       // bool: is_unicode<>
    static inline core::kas_clear _c{base_t::obj_clear};
};


// support largest int by host
// holds values that are too big for `e_fixed_t`

template <typename REF, typename VALUE>>
struct bigint_host_t : core::kas_object<bigint_host_t<REF, VALUE>, REF>
{
    using base_t      = core::kas_object<bigint_host_t<REF, VALUE>, REF>;
    using emits_value = std::true_type;
    using value_type = std::intmax_t;

    explicit bigint_host_t(value_type v = {}) : value(v) {};

    value_type const& operator()() const { return value; }

private:
    value_type value {};
    static inline core::kas_clear _c{base_t::obj_clear};
};
#endif
template <typename REF, typename VALUE, typename FMT>
auto float_host_ieee<REF, VALUE, FMT>::get_flags() const -> flag_t
{
    // get sign & zero the other flags
    flag_t flags = { std::signbit(value) };

    // look for special cases
    switch (std::fpclassify(value))
    {
        case FP_ZERO:
            flags.is_zero = true;
            break;
        case FP_INFINITE:
            flags.is_inf  = true;
            break;
        case FP_NAN:
            flags.is_nan  = true;
            break;
        case FP_SUBNORMAL:
            flags.subnorm = true;
            break;
        case FP_NORMAL:
        default:
            break;
    }
    return flags; 
}

template <typename REF, typename VALUE, typename FMT>
template <typename T>
auto float_host_ieee<REF, VALUE, FMT>::get_bin_parts(T *mantissa_p, int mant_bits, bool truncate) const
    -> std::tuple<flag_t, int>
{
    if ((unsigned)mant_bits > HOST_MANT_BITS)
        throw std::logic_error{"float_host_ieee::get_parts: bits out of range"};

    // normalized number must be positive
    auto flags    = get_flags();
    auto fraction = flags.is_neg ? -value : value;
    
    // extract normalized fraction, exponent
    int exponent;
    fraction = std::frexp(fraction, &exponent);

    // need to round if less than full precision requested
    if (mant_bits != HOST_MANT_BITS)
    {
        // round (or truncate) at N bits, then shift for export
        if (!truncate)
            fraction = std::round(std::ldexp(fraction, mant_bits));
        else
            fraction = std::trunc(std::ldexp(fraction, mant_bits));
        
        // fraction must remain < 1.0
        if (fraction >= std::exp2(mant_bits))
        {
            fraction /= 2;
            ++exponent;
        }
        
        // shift value to LSBs (of integral value)
        // NB: this reduced precision float is "sub-normal"
        fraction = std::ldexp(fraction, -HOST_MANT_BITS);
    }

    // in order convert float to fixed without using bit manipulation, must
    // save integral part, subtract, and repeat.

    // get info on host `value_type`
    static constexpr auto MANT_WORD_BITS = std::numeric_limits<T>::digits;
    static constexpr auto MANT_WORDS     = (HOST_MANT_BITS-1)/MANT_WORD_BITS + 1;

    if (mantissa_p)
    {
        auto p = mantissa_p;
        auto n = MANT_WORDS;
        while (n--)
        {
            // get bits for this word
            fraction = std::ldexp(fraction, MANT_WORD_BITS);

            // split "fraction" in to integral/fraction parts
            // NB: modf is exact: no rounding
            value_type ip;
            fraction = std::modf(fraction, &ip);

            // save integral part
            *mantissa_p++ = ip;
        }
    }
    
    return std::make_tuple(flags, exponent);
}
}

#endif
