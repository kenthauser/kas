#ifndef KAS_EXPR_FORMAT_IEEE754_IMPL_H
#define KAS_EXPR_FORMAT_IEEE754_IMPL_H 

//
// Definition of the binary IEEE format is widely available, including on Wikipedia.
// Definition of the decimal IEEE format is derived from "Decimal Arithmetic Encodings"
// by Mike Cowlishaw, retrieved from http://speleotrove.com/decimal/decbits.pdf
//
// This implementation is derived from the information at those two sources
//

#include "expr/format_ieee754.h"


namespace kas::expression
{
// implement "interchange" formats
// IEEE "single" format (32-bits)
template <typename Derived, typename FLT>
auto ieee754_base<Derived, FLT>::
    flt2(flt_t const& flt, std::integral_constant<int,32>) const
    -> result_type
{
    // args: format, exponent bits, mantissa bits, buffer
    auto p      = output.begin();
    auto mant_p = flt2(flt, FLT2_LB, 8, 24, p);
    p[0] |= *mant_p;
    return { sizeof(*p), 1, p };
}

// IEEE "double" format (64-bits)
template <typename Derived, typename FLT>
auto ieee754_base<Derived, FLT>::
    flt2(flt_t const& flt, std::integral_constant<int,64>) const
    -> result_type
{
    // args: format, exponent bits, mantissa bits, buffer
    auto p      = output.begin();
    auto mant_p = flt2(flt, FLT2_LB, 11, 53, p);
    p[0] |= mant_p[0];
    p[1]  = mant_p[1];
    return { sizeof(*p), 2, p };
}

template <typename Derived, typename FLT>
auto ieee754_base<Derived, FLT>::
    flt2(flt_t const& flt, std::integral_constant<int,16>) const
    -> result_type
{
    // args: format, exponent bits, mantissa bits, buffer
    auto p      = output.begin();
    auto mant_p = flt2(flt, FLT2_LB, 5, 11, p);

    // "half" format is only 16-bits, so need a 16-bit value to store in
    static uint16_t half_result;
    half_result  = p[0] >> 16;      // just MSBs to get expoent, etc
    half_result |= *mant_p;         // mantissa
    return { sizeof(half_result), 1, &half_result };
}

// IEEE binary floating point conversion
// common routine to generate Exponent & Mantissa.
// Final assembly required.
template <typename Derived, typename FLT>
auto ieee754_base<Derived, FLT>::
    flt2(flt_t const& flt, unsigned fmt, unsigned exp_bits, unsigned mant_bits, IEEE_DATA_T *p) const
    -> mantissa_t *
{
    // IEEE uses 32-bit groups
    using mantissa_t = uint32_t;
    static constexpr auto MANT_WORD_BITS = 32;
    static constexpr auto MANT_WORDS     = (flt_t::HOST_MANT_BITS-1)/MANT_WORD_BITS + 1;
    
    // working buffer
    static std::array<mantissa_t, MANT_WORDS> mantissa;

    // convert to fixed point format
    auto [flags, exp] = flt.get_bin_parts(mantissa.data(), mant_bits);

    //std::cout << "flt2: exp = " << std::dec << exp;
    //std::cout << " mant = " << std::hex << mantissa[0] << " " << mantissa[1];
    //std::cout << std::endl;
    
    // IEEE biases the exponent based on count of exponent bits
    auto bias = (1 << (exp_bits - 1)) - 1;
    exp += bias - 1;
    
    // if underflow, denormalize mantissa
    if (exp < 0 || (exp == 0 && fmt == FLT2_LB))
    {
        // Leading bit protocol starts denormalize at zero, not -1
        if (fmt == FLT2_LB)
            --exp;
        
        // extract mantissa with fewer bits (NB: exp is negative)
        std::tie(std::ignore, std::ignore) = flt.get_bin_parts(mantissa.data(), mant_bits + exp);

        // denormalied exponent is always zero
        exp = 0;
    }

    auto mant_begin = mantissa.end() - ((mant_bits-1)/32) - 1;

    // if Leading Bit protocol, clear bit mantissa MSB
    if (fmt == FLT2_LB)
    {
        auto msb = mant_bits - 1;

        // if MSB is bit zero of word, skip to next word
        if (msb == 0)
            ++mant_begin;
        else
            mant_begin[0] &=~ (1 << msb);
    }            

    // check for overflow
    if (exp > (2 * bias))
    {
        exp = 2 * bias + 1;
        mantissa.fill(0);
    }

    // if zero, then insert zero
    if (flags.is_zero)
    {
        exp = 0;
        // NB: mantissa should already be all 0s
    } else if (flags.is_nan)
    {
        exp = 2 * bias + 1;
        // NB: mantissa should already be all 1s
    }


    // generate exponent. 
    // NB: even 256 bit format only has 19 bits of exponent, so exponent
    // never extends past one word.

    // calculate sign & exp as 16-bits, then shift
    IEEE_DATA_T e = exp;
    if (flags.is_neg)
        e |= 1 << exp_bits;

    // left justify exponent
    *p = e << (32 - exp_bits - 1);

    //std::cout << "flt2: bias = " << std::hex << bias;
    //std::cout << " exp = " << *p;
    //std::cout << " words = " << std::end(mantissa) - mant_begin;
    //std::cout << " mant = " << mantissa[0] << " " << mantissa[1];
    //std::cout << std::endl;
    
    return mant_begin;
}

// get message if no conversion from float -> fixed
// NB: static method
template <typename Derived, typename FLT>
void ieee754_base<Derived, FLT>::ok_for_fixed(expr_t& expr, uint8_t fixed_bits)
{
    if (auto p = expr.get_p<e_float_t>())
    {
        using flt_t = typename e_float_t::object_t;

        std::cout << "ok_for_fixed: float = " << expr_t(*p) << std::endl;
        
        auto const& flt = p->get();
        print_type_name {"ok_for_fixed: *p"}(*p);
        print_type_name {"ok_for_fixed: flt"}(flt);

 //       std::cout << "flt (expr *p): " << expr_t(*p) << std::endl;
 //       std::cout << "flt (*p): " << *p << std::endl;
 //       std::cout << "flt (flt) " << flt << std::endl;
        
        // XXX why is dummy required
        uint32_t dummy[2];
#if 1
        auto [flags, exp] = flt.get_bin_parts(dummy);
        if (flags.is_inf)
            expr = e_diag_t::error(err_msg_t::ERR_flt_inf);
        else if (flags.is_nan)
            expr = e_diag_t::error(err_msg_t::ERR_flt_nan);
        else if (exp > fixed_bits)
            expr = e_diag_t::error(err_msg_t::ERR_flt_ovf);
 //       else
 //           e_diag_t::warning(err_msg_t::ERR_flt_fixed, flt.loc());
#endif
    }
}

// fixed format has general solution
template <typename Derived, typename FLT>
template <typename T>
T ieee754_base<Derived, FLT>::fixed(flt_t const& flt, T)
{
    static_assert(std::is_signed_v<T>, "ieee_754::fixed requires signed result type");
    static constexpr auto bits  = std::numeric_limits<T>::digits;
    static constexpr auto words = (bits-1)/32 + 1;
    
    // IEEE uses 32-bit groups
    using mantissa_t = uint32_t;
    static constexpr auto MANT_WORD_BITS = 32;
    static constexpr auto MANT_WORDS     = (flt_t::HOST_MANT_BITS-1)/MANT_WORD_BITS + 1;
    
    // working buffer
    std::array<mantissa_t, MANT_WORDS> mantissa;


    // many float -> fixed conversions are possible
    // 
    // This method: round float at "# + 1" of "bits" requested
    // Take integral part of resulting float.
    // NB: This makes .9 repeating resolve to 1 instead 0 that `modf` would return
    
    // round mantissa to "bits" of resolution (+1 to allow non-repeating truncation)
    auto [flags, exp] = flt.get_bin_parts(mantissa.data(), bits+1);

    // extract # of integral bits
    if (exp < 0) exp = 0;
    if (exp > bits) flags.is_inf = true;
  
    auto [_b, _c]   = flt.get_bin_parts(mantissa.data(), exp, true);   // truncate
   
    // NB: `is_signed` assert means `bits` doesn't include sign bit
    // NB: thus, can just negate for negative number
    auto p = mantissa.begin();
    auto n = words;
    T result{};
    while (n--)
    {
        result <<= 32;
        result |= *p++;
    }
    if (flags.is_neg)
        result = -result;
    return result;
}

// declare a "decimal" analog to the floating-point method
// to convert floating point to fixed point.
template <typename Derived, typename FLT>
auto ieee754_base<Derived, FLT>::
    get_dec_parts(flt_t const& flt, int precision) const
        -> std::tuple<decltype(std::declval<flt_t>().get_flags()), int, std::string>
{
#if 0
    if (precision > std::numeric_limit<typename FLT::value_type>::digits10)
        throw std::logic_error("decimal precision out-of-range");
#endif

    auto flags = flt.get_flags();

    // use stream i/o to convert float to decimal (rounding at precision)
    std::ostringstream os;

    // format float as "<optional -><single digit>.<precision digits><e><+/-><digits>"
    os << std::scientific << std::setprecision(precision-1) << flt();

    //std::cout << "get_dec_parts: " << flt() << " -> " << os.str() << std::endl;

    // decompose above to signed exponent (integral value) & begin/end pair for digits
    int exp = 0;
    auto str = os.str();
    auto end = str.end();
    auto bgn = str.begin();

    // reverse search for 'e'. NB: no 'e' in NaN or Inf
    while (end != bgn)
        if (*--end == 'e')
            break;
    
    // here extract exponent, if present
    if (end != bgn)
    {
        auto exp_iter = end;

        // get exponent sign
        auto exp_sign = *++exp_iter;

        // absorb digits
        while (auto c = *++exp_iter)
        {
            exp *= 10;
            exp += c - '0';
        }

        if (exp_sign == '-')
            exp = -exp;

        // digits: skip leading '-' & eliminate decimal point
        if (*bgn == '-')
            ++bgn;

        // get first digit
        auto c = *bgn++;

        // overwrite '.'
        *bgn = c;
        // XXX ??? need to perform subnormal checks...
    }

    // return result
    return std::make_tuple(flags, exp, std::string(bgn, end));
}

}
#endif
