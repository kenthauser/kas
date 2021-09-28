#ifndef KAS_EXPR_LITERAL_FLOAT_H
#define KAS_EXPR_LITERAL_FLOAT_H

// declare assembler FLOATING POINT literal type

// since floating point types won't fit in a 32-bit int
// (or even a 64-bit int), allocate each instance on a private deque & reference
// by index. This helps with variant & speeds copying.

// *** FLOATING POINT CONTAINER TYPE ***

// declare 2 types for literal floating point numbers:
// un-interpreted (character iterator range) & host (typically long double)

// un-interpreted is typically held until value is emitted & is then
// directly converted to emitted binary. `host` format is used when
// calculation is performed on floating point value (unusual)

#include "parser/parser_types.h"
#include "kas_core/ref_loc_t.h"
#include "kas_core/kas_object.h"
#include <boost/range/iterator_range.hpp>
#include <iomanip>

#include <limits>

namespace kas::expression::detail
{

// Declare master template
template <typename REF, typename VALUE, typename FMT, typename = void>
struct float_host_t;

// Specialize to `void` if VALUE or FMT is void
template <typename REF, typename VALUE>
struct float_host_t<REF, VALUE, void, void> : meta::id<void> {};

template <typename REF, typename FMT>
struct float_host_t<REF, void, FMT, void> : meta::id<void> {};

// Define actual type
template <typename REF, typename VALUE, typename FMT>
struct float_host_t<REF, VALUE, FMT, void> 
        : core::kas_object<float_host_t<REF, VALUE, FMT, void>, REF>
{
    using base_t = core::kas_object<float_host_t<REF, VALUE, FMT, void>, REF>;
    using type   = base_t;

    // IEEE is based on 32-bits groups, so organize mantissa into 32-bit segments
    // most significant is first. 
    using emits_value = std::true_type;     // XXX for `expr_fits`
    using value_type  = VALUE;
    using fmt_t       = FMT;
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
#if 1   
    float_host_t(value_type value = {}, parser::kas_loc loc = {})
            : value_(value), base_t(loc)
        {
            std::cout << "float_host_t::ctor: " << value << std::endl;
        }
#else
    constexpr float_host_t(value_type value = {}, parser::kas_loc loc = {})
            : value(value), base_t(loc) {}
#endif
    // operator() extracts value
    value_type const& operator()() const
    {
        return value_;
    }

    // allow standard expression operations on type 
    operator value_type const&() const
    {
        return (*this)();
    }
    
    // named method easier to call with pointer 
    auto& value() const { return (*this)(); }

    template <typename...Ts>
    auto format(Ts&&...args) const
    {
        return fmt_t().flt(value_, std::forward<Ts>(args)...);
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
    // returns "err_msg_t" or "e_fixed_t"
    template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
    auto as_fixed(T) const { return as_fixed_n(std::numeric_limits<T>::digits); }

    template <typename T>
    auto as_fixed()  const { return as_fixed(T()); }

private:
    ast::expr_t as_fixed_n(int n) const;

    value_type value_;
};

}
#endif

