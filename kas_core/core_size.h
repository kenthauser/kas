#ifndef KAS_CORE_SIZE_H
#define KAS_CORE_SIZE_H

#if 0

The assembler uses a {min,max} model to keep track of object code
instruction sizes. Three variants of this (based on the underlying type)
holding the {man & max} values.

The size types (and usages are):

op_size_t <int16_t>:
                    used to hold individual instruction size.
                    negative values for min/max used to indicate
                    error & alignment ops respectively

addr_offset_t<uint16_t>:
                    used to hold frament offset in `core_addr`.
                    value type is sized so that entire offset
                    fits in `uint32_t` field in opcode

frag_offset_t<uint32_t>
                    used to hold fragment, segment, and section sizes.


#endif

#include <ios>

namespace kas { namespace core
{

    template <typename T>
    struct offset_t {
        using OFFSET_T = T;

        constexpr offset_t(T value = 0)
            : min(value), max(value) {}

        constexpr offset_t(T min, T max)
            : min(min), max(max) {}

        // converting ctor (disallow narrowing)
        // XXX (allow narrowing...)
        template <typename U, typename = std::enable_if_t<!std::is_same<U, T>::value>>
        constexpr offset_t(offset_t<U> const& arg)
            : min(arg.min), max(arg.max)
            {}


        constexpr offset_t& operator+=(T arg)
        {
            min += arg;
            max += arg;
            return *this;
        }

        constexpr offset_t& operator-=(T arg)
        {
            min -= arg;
            max -= arg;
            return *this;
        }

        template <typename U>
        constexpr offset_t& operator+=(offset_t<U> const& arg)
        {
            max += arg.max;
            min += arg.min;
            return *this;
        }

        template <typename U>
        constexpr offset_t& operator-=(offset_t<U> const& arg)
        {
            max -= arg.max;
            min -= arg.min;
            return *this;
        }

        // result {min,max} is `min` of min's, `max` of max's
        template <typename U>
        constexpr offset_t& operator|=(offset_t<U> const& arg)
        {
            if (arg.max < 0) {
                set_error(); 
            } else if (max >= 0) {
                if (max < arg.max)
                    max = arg.max;
                if (min > arg.min)
                    min = arg.min;
            }
            return *this;
        }

        template <typename U>
        constexpr offset_t operator+(U const& arg) const
        {
            offset_t result{*this};
            return result += arg;
        }

        template <typename U>
        constexpr offset_t operator-(U const& arg) const
        {
            offset_t result{*this};
            return result -= arg;
        }

        template <typename U>
        constexpr offset_t& operator-()
        {
            min = -min;
            max = -max;
            return *this;
        }

        template <typename U>
        constexpr offset_t operator-() const
        {
            offset_t result{*this};
            return -result;
        }

        constexpr bool operator==(offset_t const& o) const
        {
            return min == o.min && max == o.max;
        }

        constexpr bool operator!=(offset_t const& o) const
        {
            return !(*this == o);
        }

        // used to check for overflow when advancing `dot`
        template <typename U>
        constexpr bool can_add(offset_t<U> const& arg) const
        {
            // calculate "room" before overflow
            auto max_room = std::numeric_limits<T>::max() - max;
            return max_room > arg.max;
        }

        constexpr auto operator()() const
        {
            return max;
        }
    
        constexpr bool is_relaxed() const { return min == max; }

        // max less than min signifies error
        constexpr bool is_error()   const { return max < min; }

        // choose value with min > max. 
        constexpr void set_error()        { *this = { 2, 0 }; }

        template <typename OS>
        friend OS& operator<< (OS& os, offset_t const& size)
        {
            return os << std::dec << "[" << size.min << "," << size.max << "]";
        }

        T min{}, max{};
    };


}}
#endif
