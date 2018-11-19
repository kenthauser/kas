#ifndef KAS_UTILITY_REDUCE_H
#define KAS_UTILITY_REDUCE_H

// implement the `reduce` algorithm
// like `accumulate`, but no no initial value

#include <utility>
#include <tuple>

namespace kas
{
	namespace detail
		{
		template <typename F, typename T>
		decltype(auto) reduce(F const& f, T&& t)
		{
			return std::forward<T>(t);
		}

		template <typename F, typename T, typename...Ts, typename = std::enable_if_t<!!sizeof...(Ts)>>
		decltype(auto) reduce(F const& f, T&& t, Ts&&...args)
		{
			return f(std::forward<T>(t), reduce(f, std::forward<Ts>(args)...));
		}
	}

	template <typename F, typename T, std::size_t...index>
	decltype(auto) reduce_tuple(F const& f, T&& t, std::index_sequence<index...>)
	{
		return detail::reduce(f, std::get<index>(std::forward<T>(t))...);
	}

	template <typename F, typename T>
	decltype(auto) reduce_tuple(F const& f, T&& t)
	{
		constexpr auto N = std::tuple_size<std::decay_t<T>>::value;
		return reduce_tuple(f, std::forward<T>(t), std::make_index_sequence<N>{});
	}
}

#endif