#ifndef KAS_IS_CONTAINER_H
#define KAS_IS_CONTAINER_H 

#include <type_traits>

namespace kas
{
// copied from
// https://stackoverflow.com/questions/12042824/how-to-write-a-type-trait-is-container-or-is-vector
// KBH 2018/7/7 updated to c++17 idioms (ie std::void_t<> & _v)

template<typename T, typename = void>
struct is_container : std::false_type {};

template<typename T>
struct is_container<T, std::void_t<
                            typename T::value_type,
                            typename T::size_type,
                            typename T::allocator_type,
                            typename T::iterator,
                            typename T::const_iterator,
                            decltype(std::declval<T>().size()),
                            decltype(std::declval<T>().begin()),
                            decltype(std::declval<T>().end()),
                            decltype(std::declval<T>().cbegin()),
                            decltype(std::declval<T>().cend())
                            >
                    > : std::true_type {};
                    
template< class T >
inline constexpr bool is_container_v = is_container<T>::value;
}
#endif
