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
