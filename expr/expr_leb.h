#ifndef KAS_EXPR_EXPR_LEB_H
#define KAS_EXPR_EXPR_LEB_H

#include <cstdint>

#if 0

LEB128 utilities.

Interface:

template <typename value_t> struct {S, U}LEB;

using READ_FN = u_char (*fn)();
using WRITE_FN = void (*fn)(u_char);

static constexpr max_value(size_n)
static constepxr max_size ()   
size_t write(WRITE_FN, value_t)
value_t read(READ_FN)


#endif


namespace kas::expression
{

// template argument is largest type to be converted
template <typename T = std::uintmax_t>
struct uleb128
{
    using value_t  = std::make_unsigned_t<T>;
    using NAME = KAS_STRING("ULEB128");

    // max value for `N` digits (NB: N > 0)
    static constexpr value_t max_value(std::size_t n)
    {
        return (1 << (n * 7)) - 1; 
    }

    static constexpr value_t min_value(std::size_t n)
    {
        return 0;
    }

    // max uleb128 digits to represent `value_t`
    static constexpr auto max_size()
    {
        constexpr auto n_bits = std::numeric_limits<value_t>::digits;
        return 1 + ((n_bits-1)/7);
    }
   
    template <typename WRITE_FN>
    static int write(WRITE_FN& fn, value_t value)
    {
        int size = 0;
        do {
            if (value >= (1 << 7))
                fn((value | 0x80) & 0xff);
            else
                fn(value);
            value >>= 7;
            ++size;
        } while(value);

        return size;
    } 

    template <typename READ_FN>
    static value_t read(READ_FN const& fn)
    {

        // make sure `fixed` is unsigned
        value_t result = 0;
        int     shift  = 0;

        for(;;) {
            auto byte = fn();
            result |= (byte & 0x7f) << shift;
            if (!(result & 0x80))
                break;
            shift += 7;
        }
        return result;
    } 
};


template <typename T = std::intmax_t>
struct sleb128
{
    using value_t  = std::make_signed_t<T>;
    using NAME = KAS_STRING("SLEB128");

    // max value for `N` digits (NB: N > 0)
    static constexpr value_t max_value(std::size_t n)
    {
        return (1 << (n * 7 - 1)) - 1; 
    }

    static constexpr value_t min_value(std::size_t n)
    {
        return (-1 << (n * 7 - 1)); 
    }

    // max sleb128 digits to represent `s_value_t`
    static constexpr auto max_size()
    {
        constexpr auto n_bits = std::numeric_limits<value_t>::digits;
        return 1 + ((n_bits-1)/7);
    }

    template <typename WRITE_FN>
    static int write(WRITE_FN& fn, value_t value)
    {
        int size = 0;
        bool done = false;
        do {
            uint8_t byte = value & 0x7f;
            bool    bit6_clear = !(byte & 0x40);
            value >>= 7;
            done = ((value ==  0 &&  bit6_clear) ||
                    (value == -1 && !bit6_clear));

            if (!done)
                byte |= 0x80;

            fn(byte);
            ++size;
        } while (!done);

        return size;
    }

    template <typename READ_FN>
    static value_t read(READ_FN const& fn)
    {

        // make sure `fixed` is unsigned
        value_t result = 0;
        int     shift  = 0;
        uint8_t byte;

        for(;;) {
            byte = fn();
            result |= (byte & 0x7f) << shift;
            if (!(result & 0x80))
                break;
            shift += 7;
        }

        // sign extend if negative
        static const auto size = std::numeric_limits<value_t>::digits;
        if ((byte & 0x40) && (shift < size))
            result |= ~0 << shift;

        return result;
    } 
};
}


#endif
