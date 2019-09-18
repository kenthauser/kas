#ifndef KAS_EXPR_FORMAT_IEEE754_H
#define KAS_EXPR_FORMAT_IEEE754_H 

//
// Definition of the binary IEEE format is widely available, including on Wikipedia.
// Definition of the decimal IEEE format is derived from "Decimal Arithmetic Encodings"
// by Mike Cowlishaw, retrieved from http://speleotrove.com/decimal/decbits.pdf
//
// This implementation is derived from the information at those two sources
//

#include "expr/literal_types.h"


namespace kas::expression
{


// declare "interchange" formatter
struct ieee754;

template <typename Derived = ieee754, typename FLT = detail::float_host_t>
struct ieee754_base
{
    using base_t    = ieee754_base;
    using derived_t = Derived;
    using flt_t     = FLT;

    // chunk format used for data
    using IEEE_DATA_T = uint32_t;

    // floating point format result type:
    // tuple: { bytes_per_chunk, chunk_count, chunk_pointer }
    using result_type = std::tuple<uint8_t, uint8_t, void const *>;

    // conversion target formats
    // NB: FLT2_LB uses the "Leading Bit" protocol
    enum { FLT2, FLT2_LB, FLT10, FLT_FIXED };

    // interface funtions: fixed always return integral type. Specify return type.
    template <typename T = long>
    T fixed(FLT const& flt) const
    {
        return derived().fixed(flt, T());
    }
    
    // process IEEE interchange formats
    result_type flt(FLT const& flt, int bits) const
    {
        switch (bits)
        {
            case 32:
                return derived().flt2(flt, std::integral_constant<int, 32>());
            case 64:
                return derived().flt2(flt, std::integral_constant<int, 64>());
            default:
                break;
        }
        return { 0, 0, "Unsupported floating point format" };
    }

//protected:
    // CRTP casts
    auto& derived() const
        { return *static_cast<derived_t const*>(this); }
    auto& derived()
        { return *static_cast<derived_t*>(this); }

    // declare largest output format
    static constexpr auto MAX_OUTPUT_BITS  = 128;
    static constexpr auto MAX_OUTPUT_WORDS = (MAX_OUTPUT_BITS-1)/32 + 1;

    // shared output buffer
    static inline std::array<IEEE_DATA_T, MAX_OUTPUT_WORDS> output;
    
    // no general IEEE format: return not supported
    template <typename T>
    result_type flt2(FLT const& flt, T) const
    {
        // not supported
        //return { 0, "Unsupported IEEE format" };
        return { 0, 0, nullptr };
    }

    template <typename T>
    result_type flt10(FLT const& flt, T) const
    {
        // not supported
        //return { 0, "Unsupported IEEE format" };
        return { 0, 0, nullptr };
    }

    // fixed format has general solution
    template <typename T>
    T fixed(FLT const& flt, T) const
    {
        static constexpr auto bits  = std::numeric_limits<T>::digits;
        static constexpr auto words = (bits-1)/32 + 1;
        
        // IEEE uses 32-bit groups
        static_assert(FLT::MANT_WORD_BITS == 32);

        // many float -> fixed conversions are possible
        // 
        // This method: round float at # of "bits" requested
        // Take integral part of resulting float.
        // NB: This makes .9 repeating resolve to 1 instead 0 that `modf` would return
        
        // round mantissa to "bits" resolution
        auto [flags, exp, _a] = flt.get_bin_parts(bits);

        // extract # of integral bits
        if (exp < 0) exp = 0;
        auto [_b, _c, mant]   = flt.get_bin_parts(exp);
       
        // XXX temp
        auto p = mant.end();
        auto n = words;
        T result{};
        while (n--)
        {
            result <<= sizeof(IEEE_DATA_T);
            result |= *--p;
        }
        return result;
    }

    // IEEE "single" format (32-bits)
    result_type flt2(FLT const& flt, std::integral_constant<int,32>) const
    {
        // args: format, exponent bits, mantissa bits, buffer
        auto p      = output.begin();
        auto mant_p = flt2(flt, FLT2_LB, 8, 24, p);
        p[0] |= *mant_p;
        return { sizeof(*p), 1, p };
    }
    
    // IEEE "double" format (64-bits)
    result_type flt2(FLT const& flt, std::integral_constant<int,64>) const
    {
        // args: format, exponent bits, mantissa bits, buffer
        auto p      = output.begin();
        auto mant_p = flt2(flt, FLT2_LB, 11, 53, p);
        p[0] |= mant_p[0];
        p[1]  = mant_p[1];
        return { sizeof(*p), 2, p };
    }

    // IEEE binary floating point conversion
    // common routine to generate Exponent & Mantissa.
    // Final assembly required.
    auto flt2(FLT const& flt, unsigned fmt, unsigned exp_bits, unsigned mant_bits, IEEE_DATA_T *p) const
    {
        // IEEE uses 32-bit groups
        static_assert(FLT::MANT_WORD_BITS == 32);

        // convert to fixed point format
        auto [flags, exp, mant] = flt.get_bin_parts(mant_bits);

        //std::cout << "flt2: exp = " << std::dec << exp;
        //std::cout << " mant = " << std::hex << mant[0] << " " << mant[1];
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
            std::tie(std::ignore, std::ignore, mant) = flt.get_bin_parts(mant_bits + exp);

            // denormalied exponent is always zero
            exp = 0;
        }

        auto mant_begin = mant.end() - ((mant_bits-1)/32) - 1;

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
            mant = {};          // set mantissa to zero
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
        //std::cout << " words = " << mant.end() - mant_begin;
        //std::cout << " mant = " << mant[0] << " " << mant[1];
        //std::cout << std::endl;
        
        return mant_begin;
    }

    // declare a "decimal" analog to the floating-point method
    // to convert floating point to fixed point.
    auto get_dec_parts(FLT const& flt, int precision) const
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
    
};

// ieee754 with just interchange formats
struct ieee754 : ieee754_base<ieee754> {};


}
#endif
