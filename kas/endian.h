#ifndef KAS_ENDIAN_H
#define KAS_ENDIAN_H

// from cppreference.com : proposed for c++20
#ifdef __cpp_lib_endian
#include <bit>
#else
namespace std 
{
enum class endian
{
#ifdef _WIN32
    little = 0,
    big    = 1,
    native = little
#else
    little = __ORDER_LITTLE_ENDIAN__,
    big    = __ORDER_BIG_ENDIAN__,
    native = __BYTE_ORDER__
#endif
};
}
#endif
#endif
