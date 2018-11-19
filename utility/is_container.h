#ifndef KAS_UTILITY_IS_CONTAINER_H
#define KAS_UTILITY_IS_CONTAINER_H

#include <type_traits>

namespace detail {
	template<typename... Ts>
	struct sfinae {};
}

template<typename T, typename = void>
struct is_container : std::false_type {};

template<typename T>
struct is_container<
        T,
        std::conditional_t<
            false,
            detail::sfinae
            	< typename T::value_type
                , typename T::size_type
                , typename T::allocator_type
                , typename T::iterator
                , typename T::const_iterator
                , decltype(std::declval<T>().size())
                , decltype(std::declval<T>().begin())
                , decltype(std::declval<T>().end())
                >,
            void
            >
        > : std::true_type {};

template <typename T>
using is_container_t = typename is_container<T>::type;

template <typename T>
constexpr bool is_container_v = is_container<T>::value;

#endif
