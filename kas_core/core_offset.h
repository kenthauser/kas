#ifndef KAS_CORE_OFFSET_H
#define KAS_CORE_OFFSET_H

/*
 * core_offset
 * 
 * The assembler uses a {min,max} model to keep track of object sizes,
 * and thus addresses.
 *
 * Three principle instantiations of `offset_t` are as follows:
 *
 * 1. `op_size_t`: size of an opcode = offset_t<int16_t>
 *
 * As instructions are coverted to `core::opcodes` the generated size is
 * stored in the `insn_container`. Depending on addressing modes, etc,
 * the final "relaxed" size may not be know until later. The size is stored
 * as a {min,max} pair. An "error" size is indicated by "min" exceeding "max".
 *
 * NB: this `type` is further *extended* in "opcode_data.h"
 *
 * 2. `frag_offset_t`: offset within a fragment = offset_t<uint16_t>
 *
 * As instructions are converted to `core::opcodes`, labels are stored in
 * the `insn_containers` as normal `opcodes`. Each opcode in the container
 * has a 32-bit "fixed" location with format defined by opcode. For `opc_label`
 * instructions, the fixed area holds a `frag_offset_t`, which is the offset
 * from the beginning of the containing fragment to the current location..
 * This value is updated during each "relax" pass. This allows addresses
 * to resolve.
 *
 * 3. `size_offset_t`: offset in address space = offset_t<uint64_t>
 *
 * Used to hold size of any fragment, segment, or section. Differs from
 * `size_t` in that it is a {min,max} pair.
 *
 * 4. `expr_offset_t`: XXX does this make sense?
 *
 * Special methods:
 *  operator{+,-}{,=}: as expected
 *  operator|=       : return {min of min, max of max}
 *  operator()       : return `max`
 *  is_relaxed()     : return { min == max }
 *  is_error()       : return { min >  max }
 *  set_error()      : set max = min; set min to numeric_limits<>:max
 *
 * Type declarations are found in `core_terminal_types.h`
 *
 * See `insn_inserter.h` for usage of `frag_offset_t`
 * See `core_addr.h` for more information on how `frag_offset_t` instances
 * are converted to addresses used by assembler.
 */

#include <limits>       // std::numeric_limits<>
#include <ios>          // std::dec

namespace kas::core
{

template <typename T>
struct offset_t
{
    using value_t  = T;

    static constexpr auto max_limit = std::numeric_limits<T>::max();

    constexpr offset_t(T min, T max) : min(min), max(max) {}
    constexpr offset_t(T min = {})   : offset_t(min, min) {}

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
    static constexpr offset_t ERROR() { return  {2, 0}; };

    void set_error()
    {
        max = min;          // save "actual" min
        min = std::numeric_limits<T>::max();
    };

    template <typename OS>
    friend OS& operator<< (OS& os, offset_t const& obj)
    {
        return os << std::dec << "[" << obj.min << "," << obj.max << "]";
    }

    T min, max;
};


}
#endif
