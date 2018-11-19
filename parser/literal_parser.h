#ifndef PARSER_LITERAL_PARSER_H
#define PARSER_LITERAL_PARSER_H

#include <boost/spirit/home/x3/char/literal_char.hpp>
#include <boost/spirit/home/x3/string/literal_string.hpp>
#include <boost/mpl/string.hpp>
// #include <boost/mpl/identity.hpp>

#include "utility/print_type_name.h"

namespace kas { namespace parser
{
	namespace x3 = boost::spirit::x3;

	using char_ns = boost::spirit::char_encoding::standard;
	using Iter    = char const *;

	template <typename T>
	struct fixed_char_parser : x3::literal_char<char_ns> {
		fixed_char_parser() :
			literal_char<char_ns>(*boost::mpl::c_str<T>::value)
			{}
	};

	template <typename T>
	struct fixed_string_parser : x3::literal_string<Iter, char_ns> {
		fixed_string_parser() :
			literal_string<Iter, char_ns>(boost::mpl::c_str<T>::value)
		{}
	};

	template <typename T, int N = boost::mpl::size<typename T::type>::value>
	struct literal_parser : fixed_string_parser<T> {};

	template<typename T>
	struct literal_parser<T, 1> : fixed_char_parser<T> {};

	template<typename T>
	struct literal_parser<T, 0> : boost::mpl::identity<void> {};
}}


#endif
