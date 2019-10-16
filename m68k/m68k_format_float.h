#ifndef KAS_M68K_M68K_FORMAT_FLOAT_H
#define KAS_M68K_M68K_FORMAT_FLOAT_H

//
// The m68k floating point coprocesser uses four "floating point" formats
//
// 1. IEEE single precision (32-bit)
// 2. IEEE double precision (64-bit)
// 3. M68K extended precision (80-bit)
// 4. M68K packed decimal (80-bit)
//
// The IEEE formats are supported by the routines in "expr/format_ieee754.h"
//
// Add support for the M68K specific types (extended, packed)
//

#include "expr/format_ieee754_impl.h"


namespace kas::m68k
{

struct m68k_format_float : expression::ieee754_base<m68k_format_float, e_float_t>
{
    using base_t      = expression::ieee754_base<m68k_format_float, e_float_t>;

    enum
    {
          FMT_M68K_BASE = NUM_FMT_IEEE
        , FMT_M68K_80_EXTEND
        , FMT_M68K_80_PACKED
    };
    

    result_type flt(flt_t const& flt, int bits) const
    {
        switch (bits)
        {
            case FMT_M68K_80_EXTEND:
                return flt2(flt, std::integral_constant<int, 80>());
            case FMT_M68K_80_PACKED:
                return m68k_packed(flt);
            default:
                return base_t::flt(flt, bits);
        }
    }; 

//protected:
    // M68K "extended" format (80-bits)
    // add 80-bit binary overload. Expose existing overloads.
    using base_t::flt2;
    
    result_type flt2(flt_t const& flt, std::integral_constant<int,80>) const
    {
        // args: format, exponent bits, mantissa bits, buffer
        auto p      = output.begin();
        auto mant_p = flt2(flt, base_t::FLT2, 15, 64, p);
        p[1] = mant_p[0];
        p[2] = mant_p[1];
        return { sizeof(*p), 3, p };
    }

    // M68K "packed" output format (80-bits)
    result_type m68k_packed(flt_t const& flt) const
    {
        auto [ flags, exp, digits ] = get_dec_parts(flt, 17);

        // check for exponent overflow/underflow
        if (exp > 999)
            flags.is_inf = true;
        else if (exp < -999)
            flags.is_zero = true;

        auto p = output.begin();
    
        // Generate a NaN
        if (flags.is_nan)
        {
            // IEEE uses 32-bit groups
            using mantissa_t = uint32_t;
            static constexpr auto MANT_WORD_BITS = 32;
            static constexpr auto MANT_WORDS     = (flt_t::HOST_MANT_BITS-1)/MANT_WORD_BITS + 1;
            
            // working buffer
            mantissa_t mantissa[MANT_WORDS];

            // generate a "NaN" with sign bit clear
            static_assert(std::is_unsigned_v<std::remove_reference_t<decltype(*p)>>);
            *p++ = ~0 >> 1;

            // generate a NaN with "payload" bits from `flt`
            // NB: get "full" precision to prevent rounding
            // NB: correct for 64-bit mantissa host. Unclear about other combinations
            auto [_1, _2] = flt.get_bin_parts(mantissa);
            *p++ = mantissa[0];
            *p++ = mantissa[1];
        }
       
        // Generate a zero value
        else if (flags.is_zero)
        {
            output.fill(0);
            if (flags.is_neg)
                *p = 1 << 31;   // flag -0.0
        }

        // Generate +/- Inf
        else if (flags.is_inf)
        {
            uint16_t code = ~0;
            if (!flags.is_neg)
                code >>= 1;     // clear sign
            
            output.fill(0);     // clear all digits
            *p = code << 16;    // save +/- Inf code
        }

        // Normal case: Generate "in-range"
        else
        {
            // get mantissa & exponent signs
            uint32_t code = flags.is_neg ? 8 : 0;
            if (exp < 0)
            {
                code |= 4;
                exp   = -exp;
            }
            code <<= 4;

            // brute force decimal to packed BCD conversion. 
            // all values 000-999 allowed

            for (; exp >= 100; exp -= 100)
                ++code;
            code <<= 4;

            for (; exp >= 10; exp -= 10)
                ++code;
            code <<= 4;

            code += exp;
        
            code <<= 12;  // leave room to shift in integer

            // shift in digits. 1 digit first word, 8 digits each of next two
            auto digit_p   = digits.begin();
            auto digit_cnt = 1;
            auto word_cnt  = 3;

            while (word_cnt--)
            {
                while (digit_cnt--)
                {
                    code <<= 4;
                    if (*digit_p)
                        code += *digit_p++ - '0';

                }
                *p++ = code;
                code = 0;
                digit_cnt = 8;
            }
        }

        return { sizeof(*p), 3, output.begin() };
    }
    
};

}
#endif
