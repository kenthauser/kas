#ifndef KAS_EXPR_LITERAL_TYPES_H
#define KAS_EXPR_LITERAL_TYPES_H

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
struct float_host_ieee: core::kas_object<float_host_ieee<REF, VALUE, FMT>, REF>
{
    using base_t = core::kas_object<float_host_ieee<REF, VALUE, FMT>, REF>;

    // IEEE is based on 32-bits groups, so organize mantissa into 32-bit segments
    // most significant is first. 
    using emits_value = std::true_type;     // XXX for `expr_fits`
    using value_type  = VALUE;
    using fmt         = FMT;
    using mantissa_t  = std::uint32_t;

    // if host not `ieee` format, need to implement appropriate `get_flags` & `get_bin_parts` 
    static constexpr auto HOST_MANT_BITS = std::numeric_limits<value_type>::digits;
    
    // info on floating point value
    struct flag_t
    {
        uint8_t is_neg  : 1;
        uint8_t is_zero : 1;
        uint8_t is_inf  : 1;
        uint8_t is_nan  : 1;
        uint8_t subnorm : 1;
    };
    
    constexpr float_host_ieee(value_type value = {}) : value(value) {}

    // operator() extracts value
    value_type const& operator()() const
    {
        return value;
    }

    // allow standard expression operations on type 
    operator value_type const&() const
    {
        return (*this)();
    }

//protected:
    // identify special values
    flag_t get_flags() const;

    // get_bin_parts: the floating-point "money" function
    //
    // convert floating point to fixed format, with rounding
    //
    // round mantissa to "n-bits" & store in provided array.
    // Return flags & (unbiased) exponent

    template <typename T>
    auto get_bin_parts(T *mantissa_p = {}, int mant_bits = HOST_MANT_BITS, bool truncate = false) const
        -> std::tuple<flag_t, int>;

    // convert float to n-bit fixed
    // generate diagnostic if float can't be converted to fixed
    // generate warning if it can
    template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
    auto as_fixed(T) const { return as_fixed_n(std::numeric_limits<T>::digits); }

    template <typename T>
    auto as_fixed()  const { return as_fixed(T()); }

    // returns "err_msg_t" or "e_fixed_t"
    // XXX no loc for error msg
    ast::expr_t as_fixed_n(int n) const;

private:
    value_type value;
};

  // support largest int by host
// holds values that are too big for `e_fixed_t`
#if 0
template <typename REF>
struct bigint_host_t : core::kas_object<bigint_host_t<REF>, REF>
{
    using base_t      = core::kas_object<bigint_host_t<REF>, REF>;
    using emits_value = std::true_type;
    using value_type = std::intmax_t;

    explicit bigint_host_t(value_type v = {}) : value(v) {};

    value_type const& operator()() const { return value; }

private:
    value_type value {};
    static inline core::kas_clear _c{base_t::obj_clear};
};
#endif
}
#endif

