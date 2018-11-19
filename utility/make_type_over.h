#ifndef KAS_UTILITY_MAKE_TYPE_OVER_H
#define KAS_UTILITY_MAKE_TYPE_OVER_H

//
// make_type_over <Template, MPL::Sequence>
//
// use mpl::sequence to create c++ variadic templated type
//
// usage: using T = make_type_over<VARIADIC_TEMPLATE, MPL_Sequence>;
//

#include <boost/mpl/begin_end.hpp>
#include <boost/mpl/deref.hpp>
#include <boost/mpl/next.hpp>
#include <boost/mpl/size.hpp>
#include <boost/mpl/at.hpp>
#include <meta/meta.hpp>

namespace kas
{
	namespace detail_mpl {
		// NB: in detail namespace, all types are ``mpl``
		using namespace boost::mpl;

		template <template <typename...> class OP, typename It, int N, typename...Ts>
		struct make_type_over_impl : make_type_over_impl<
					  OP
					, typename next<It>::type
					, N - 1
					, Ts...
					, typename deref<It>::type
					> {};

		template <template <typename...> class OP, typename It, typename...Ts>
		struct make_type_over_impl<OP, It, 0, Ts...>
		{
			using type = OP<Ts...>;
		};

		template <template <typename...> class OP, typename Seq>
		using make_type_over = typename make_type_over_impl<
					  OP
					, typename begin<Seq>::type
					, size<Seq>::value
					>::type;

		template <typename Seq, typename F, std::size_t...Is>
		auto make_value_tuple_impl(F&& fn, std::index_sequence<Is...>)
		{
			return std::make_tuple(fn(typename meta::at_c<Seq, Is>::type{})...);
		}

		template <typename Seq, typename F, std::size_t N = Seq::size()>
		auto make_value_tuple(F&& fn)
		{
			return make_value_tuple_impl<Seq>(std::forward<F>(fn), std::make_index_sequence<N>{});
		}

		// defaulting FN doesn't work for some reason -- just overload
		// template <typename Seq>
		// auto make_value_tuple()
		// {
		// 	auto fn = [](auto&& e) { return std::forward<decltype(e)>(e); };
		// 	return make_value_tuple<Seq>(fn);
		// }
	}

	// expose in kas:: namespace
	using detail_mpl::make_type_over;
	using detail_mpl::make_value_tuple;
}
#endif
