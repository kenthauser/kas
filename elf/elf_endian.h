#ifndef KAS_ELF_ELF_ENDIAN_H
#define KAS_ELF_ELF_ENDIAN_H

// Convert data ENDIAN between host and target formats

// swap_endian() type swaps values if needed

#include "elf_common.h"

#include <type_traits>
#include "kas/endian.h"     // as of c++20, use <bit>

namespace kas::elf
{
struct swap_endian
{
    const bool passthru;

    constexpr swap_endian(std::endian TARGET, std::endian HOST = std::endian::native)
        : passthru (TARGET == HOST) {}
    
    constexpr auto operator()(uint64_t value) const
    {
        if (passthru)       // not constexpr...
            return value;
        else
        {
            value = (value & 0x0000'0000'FFFF'FFFF) << 32
                  | (value & 0xFFFF'FFFF'0000'0000) >> 32;
            value = (value & 0x0000'FFFF'0000'FFFF) << 16
                  | (value & 0xFFFF'0000'FFFF'0000) >> 16;
            value = (value & 0x00FF'00FF'00FF'00FF) << 8
                  | (value & 0xFF00'FF00'FF00'FF00) >> 8;
            return value;
        }
    }
    constexpr auto operator()(uint32_t value) const
    {
        if (passthru)
            return value;
        else
        {
            value = (value & 0x000'0FFFF) << 16
                  | (value & 0xFFF'F0000) >> 16;
            value = (value & 0x00F'F00FF) << 8
                  | (value & 0xFF0'0FF00) >> 8;
            return value;
        }
    }
    constexpr auto operator()(uint16_t value) const
    {
        if (passthru)
            return value;
        else
        {
            value = (value & 0x00FF) << 8
                  | (value & 0xFF00) >> 8;
            return value;
        }
    }
    constexpr auto operator()(uint8_t value) const
    {
        return value;
    }

    // forward signed types thru unsigned converter
    template <typename T>
    constexpr auto operator()(T value) const
        { return (*this)(std::make_unsigned_t<T>(value)); }
   
    // swap memory based on widths
    using data_iter = void const *;
    void const *operator()(data_iter& p, uint8_t width) const
    {
        switch(width)
        {
            case 8:
            {
                static uint64_t data64;
                data64 = (*this)(*static_cast<uint64_t const *>(p));
                p = static_cast<char const *>(p) + 8;
                return &data64;
            }
            case 4:
            {
                static uint32_t data32;
                data32 = (*this)(*static_cast<uint32_t const *>(p));
                p = static_cast<char const *>(p) + 4;
                return &data32;
            }
            case 2:
            {
                static uint16_t data16;
                data16 = (*this)(*static_cast<uint16_t const *>(p));
                p = static_cast<char const *>(p) + 2;
                return &data16;
            }
            case 1:
            {
                auto s = p;
                p = static_cast<char const *>(p) + 1;
                return s;
            }
            case 0:
            {
                return nullptr;
            }
            default:
                throw std::logic_error("elf::swap_endian: invalid width: "
                                                + std::to_string(width));
        }
    }

    // convert type, then swap endian & return pointer
    void const *operator()(int64_t value, uint8_t width) const
    {
        switch (width)
        {
            case 8:
                {
                    static uint64_t data64; 
                    data64 = (*this)(static_cast<uint64_t>(value));
                    return &data64;
                }
            case 4:
                {
                    static uint32_t data32;
                    data32 = (*this)(static_cast<uint32_t>(value));
                    return &data32;
                }
            case 2:
                {
                    static uint16_t data16;
                    data16 = (*this)(static_cast<uint16_t>(value));
                    return &data16;
                }
            case 1:
                {
                    static uint8_t data8;
                    data8 = (*this)(static_cast<uint8_t>(value));
                    return &data8;
                }
            case 0:
                return nullptr;
        }
        throw std::logic_error{"elf::swap_endian: invalid width"};
    }
};
}

#endif
