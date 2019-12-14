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


namespace kas::expression::detail
{
// declare "interchange" formatter
template <typename Derived, typename FLT>
struct ieee754_base
{
    using base_t    = ieee754_base;
    using derived_t = Derived;
    using flt_t     = typename FLT::object_t;

    // declare supported formats
    enum 
    {
          FMT_IEEE_NONE
        , FMT_IEEE_32_SINGLE
        , FMT_IEEE_64_DOUBLE
        , FMT_IEEE_16_HALF
        , NUM_FMT_IEEE
    };

    // chunk format used for data
    using IEEE_DATA_T = uint32_t;       // format handed to ELF
    using mantissa_t  = uint32_t;       // mantissa buffer chunk size

    // floating point format result type:
    // tuple: { bytes_per_chunk, chunk_count, chunk_pointer }
    using result_type = std::tuple<uint8_t, uint8_t, void const *>;

    // conversion target formats
    // NB: FLT2_LB uses the "Leading Bit" protocol
    enum { FLT2, FLT2_LB, FLT10, FLT_FIXED };

    // interface funtions: fixed always return integral type. Specify return type.
    template <typename T = long>
    static T fixed(flt_t const& flt)
    {
        return derived_t::fixed(flt, T());
    }
    
    // error messages for NAN, INF, overflow
    // `expr` converted to error if `error`
    static void ok_for_fixed(expr_t& expr, uint8_t fixed_bits);
    
    // process IEEE interchange formats
    result_type flt(flt_t const& flt, int fmt) const
    {
        switch (fmt)
        {
            case FMT_IEEE_16_HALF:
                return derived().flt2(flt, std::integral_constant<int, 16>());
            case FMT_IEEE_32_SINGLE:
                return derived().flt2(flt, std::integral_constant<int, 32>());
            case FMT_IEEE_64_DOUBLE:
                return derived().flt2(flt, std::integral_constant<int, 64>());
            default:
                break;
        }
        return { 0, 0, "Unsupported floating point format" };
    }

protected:
    // CRTP casts
    auto& derived() const
        { return *static_cast<derived_t const*>(this); }
    auto& derived()
        { return *static_cast<derived_t*>(this); }

    // declare largest output format
    static constexpr auto OUTPUT_BITS_WORD = std::numeric_limits<IEEE_DATA_T>::digits;
    static constexpr auto MAX_OUTPUT_BITS  = 128;
    static constexpr auto MAX_OUTPUT_WORDS = (MAX_OUTPUT_BITS-1)/OUTPUT_BITS_WORD + 1;

    // shared output buffer
    static inline std::array<IEEE_DATA_T, MAX_OUTPUT_WORDS> output;
    
    // IEEE "single" format (32-bits)
    result_type flt2(flt_t const& flt, std::integral_constant<int,32>) const;
    
    // IEEE "double" format (64-bits)
    result_type flt2(flt_t const& flt, std::integral_constant<int,64>) const;

    // IEEE "half" format (16-bits)
    result_type flt2(flt_t const& flt, std::integral_constant<int,16>) const;
    

    // IEEE binary floating point conversion
    // common routine to generate Exponent & Mantissa.
    // Final assembly of exponent and mantissa required.
    mantissa_t *flt2(flt_t const& flt, unsigned fmt, unsigned exp_bits, unsigned mant_bits, IEEE_DATA_T *p) const;
    
    // fixed format has general solution
    template <typename T> static T fixed(flt_t const& flt, T);// const;

    // declare a "decimal" analog to the floating-point method
    // to convert floating point to fixed point.
    auto get_dec_parts(flt_t const& flt, int precision) const
        -> std::tuple<decltype(std::declval<flt_t>().get_flags()), int, std::string>;
};

// ieee754 with just interchange formats
struct ieee754 : ieee754_base<ieee754, e_float_t> {};


}
#endif
