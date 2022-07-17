#ifndef KAS_EXPR_C_FLOAT_PARSER_H
#define KAS_EXPR_C_FLOAT_PARSER_H

// define x3 parser for floating point literal

// Note: floating point literals are created from four
// components: sign, mantissa, exponent, and "base"
//
// Each of these elements is parsed separately & passed to
// the floating point literal constructor to initialize object

#include "literal_float.h"

#include <stdexcept>
#include <cassert>
#include <cmath>

namespace kas::expression::literal
{

// create a native X3 parser
template <typename T>
struct c_float_parser : x3::parser<c_float_parser<T>>
{
    using attribute_type = T;
    using object_type    = T;

    static bool const has_attribute = true;
    
    // Allow upto 64-bit mantissa.
    // This allows double & extended precision, but not quadruple and up
    using mant_t = uint64_t;        // type for mantissa

    // parsing c++14 integers. define separator character
    static constexpr auto cpp14_sep = '\'';

    // character format parsed
    struct flt_fmt
    {
        bool    has_mant;
        bool    has_dot;
        bool    has_exp;
        bool    is_hex;
    };


    template <typename Iterator, typename Context, typename Attribute_>
    bool parse(Iterator& first, Iterator const& last
      , Context const& context, unused_type, Attribute_& attr) const
    {
        x3::skip_over(first, last, context);
        Iterator it(first);     // copy iterator

        // look for (and consume) leading sign
        bool neg = *it == '-';
        if (neg || *it == '+')
            if (++it == last)
                return false;   // need more than just sign
       
        // all floating point constants begin with a digit (or dot)
        if (!std::isdigit(*it) && (*it != '.'))
            return false;

        // decode valid c++ floating point literals into mantissa & exponent
        mant_t  mantissa{};
        int     exponent{};
        flt_fmt fmt{};

        if (!parse_mant(context, it, last, mantissa, exponent, fmt))
            return false;
        if (!parse_exp(context, it, last, exponent, fmt))
            return false;
        
        // absorb floating point suffix if present
        if (it != last)
            switch (*it)
            {
                case 'l': case 'L':
                case 'f': case 'F':
                    ++it;
                    break;
                default:
                    break;
            }

        
        // compute value as long double (use `powl`)
        // g++-11 doesn't have `powl` defined. Use floating point bases
        auto value  = std::pow(fmt.is_hex ? 2.0L : 10.0L, exponent);
             value *= mantissa;

        traits::move_to(value, attr);
        first = it;             // consume parsed value
        return true;
    }
    
    // parse numeric sequence
    template <typename Context, typename Iterator>
    bool parse_mant(Context const& context, Iterator& it, Iterator const& last
                  , mant_t& mant, int& exp, flt_fmt& fmt) const
    {
        bool mant_present{};

        // look for c++-language prefix (ie: 0x)
        // otherwise parse as integral part of fraction (before `.`)
        const char *fail{};
        fmt.has_mant = true;        // know first character is numeric
        if (*it == '0')
            if (++it != last)
                switch (*it)        // no octal floats
                {
                    case 'x': case 'X':
                        fmt.is_hex = true;

                        // consume `0x`
                        if (++it == last)
                            return false;   // invalid format

                        // initial zero consumed. Check if `mant` present
                        fmt.has_mant = std::isxdigit(*it);
                        break;
                    default:
                        break;
                }

        // if `0x` prefix, parse as binary, else decimal
        if (fmt.is_hex)
            fail = _str2int<16, cpp14_sep>(it, last, mant);
        else
            fail  = _str2int<10, cpp14_sep>(it, last, mant);

        // check for fractional part
        if (*it == '.')
        {
            ++it;               // skip dot
            fmt.has_dot = true;

            // has_mant can only be clear for hex formats
            if (!fmt.has_mant && it != last)
                fmt.has_mant = std::isxdigit(*it);
            
            // require zero digits after dot (eg 15. makes a float)
            // NB: decr exponent for every fractional digit
            if (fmt.is_hex)
                fail = _str2int<16, cpp14_sep>(it, last, mant, &exp, -1, 0);
            else
                fail = _str2int<10, cpp14_sep>(it, last, mant, &exp, -1, 0);
        }

        // If floating point suffix `f` present, pretend we saw `dot`.
        // Thus `10.` and `10f` are equivalent
        // NB: don't absorb `f` suffix.
        if (it != last)
            if (*it == 'f' || *it == 'F')
                fmt.has_dot = true;

        if (fmt.is_hex)
            exp *= 4;       // each digit is four binary bits

        if (!fmt.has_mant)
            fail = "X no mantissa";
        
        // if parse failed, don't need to check exponent
        return !fail;
    }

    // parse character sequence
    template <typename Context, typename Iterator>
    bool parse_exp(Context const& context, Iterator& it, Iterator const& last
            , int& exp, flt_fmt fmt) const
    {
        // check to see if exponent present
        if (it != last)
        {
            if (fmt.is_hex)
                fmt.has_exp = (*it == 'p') || (*it == 'P');
            else
                fmt.has_exp = (*it == 'e') || (*it == 'E');
        }

        // floating point constants require either dot or exponent
        if (!fmt.has_exp)
            return fmt.has_dot;

        // has exponent: look for (and consume) leading sign
        if (++it == last)
            return false;       // `exponent` with no exponent
        bool neg = *it == '-';
        if (neg || *it == '+')
            if (++it == last)
                return false;   // sign with no exponent

        // parse exponent into `unit64_t`
        const char *fail{};
        mant_t value{};

        // default requires at least one digit (else E_too_short)
        if (fmt.is_hex)
            fail = _str2int<16, cpp14_sep>(it, last, value);
        else
            fail = _str2int<10, cpp14_sep>(it, last, value);

        // modify exponent
        if (neg)
            exp -= value;
        else
            exp += value;

        return !fail;
    }
};

}

#endif

