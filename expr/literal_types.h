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

template <typename REF>
struct kas_string_t : core::kas_object<kas_string_t<REF>, REF>
{
    using base_t      = core::kas_object<kas_string_t<REF>, REF>;
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

#define FLOAT_HOST float_host_ieee

template <typename REF, typename VALUE, typename FMT>
struct FLOAT_HOST: core::kas_object<FLOAT_HOST<REF, VALUE, FMT>, REF>
{
    using base_t = core::kas_object<FLOAT_HOST<REF, VALUE, FMT>, REF>;

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
    
    constexpr FLOAT_HOST(value_type value = {}) : value(value) {}

    // operator() extracts value
    value_type const& operator()() const
    {
        return value;
    }

    // allow standard expression operations on type 
    operator value_type const&() const
    {
        return value;
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

private:
    value_type value;
};
#if 0
// overload with type that requires ieee_754 host format
template <typename REF, typename VALUE, typename FMT>
struct float_host_ieee : float_host<REF, VALUE, FMT>
{
    static_assert(std::numeric_limits<VALUE>::is_iec559);
    
    using base_t = float_host<REF, VALUE, FMT>;
   
    // inherit constructor
    using base_t::base_t;

    // expose protected methods after proper operation verified
    using base_t::get_flags;
    using base_t::get_bin_parts;

    // expose methods used by `ref_loc_t`
    using base_t::get;
};

#endif
#undef FLOAT_HOST

}
#if 1
namespace kas::expression
{
// decalare
// XXX 
using kas_bigint_host = core::ref_loc_t<detail::bigint_host_t>;
using kas_string      = core::ref_loc_t<detail::kas_string_t>;
}
#endif

#endif
