#ifndef KAS_STRING_H
#define KAS_STRING_H

// Support for using string literals as types
//
// String literals can not be directly used as template arguments because
// they have neither a `name` nor `linkage`. Thus there are two options
// for converting string literals to types.
//
// 1. Don't use a string literal, but rather a sequence of `chars` or multi-char `ints`
//    This is the approach taken by `boost::mpl::string`. This approach works for c++99
//
// 2. Use a preprocessor macro to convert the string literal to a sequence of `chars`
//    which is then passed to a standard c++ type template, which is then TMP'd into
//    the appropriate type. This is the approach taken by `boost::metaparse`
//
// This module implements a simplified version of the `boost::metaparse` approach
// Strings are limited to 16 characters, excluding the NULL.

#include <utility>
#include <array>
#include <meta/meta.hpp>

namespace kas{}
namespace kas::string
{

//
// Define string type & nullptr specialization 
//

template <typename T = char, T...Cs>
struct kas_string
{   
public:
    using type      = kas_string;
    using char_type = T;

    constexpr kas_string() {}

    // include terminating null character 
    // NB: kas_string<nullptr>::size returns zero
    static constexpr auto size = sizeof...(Cs) + 1;

private:
    static constexpr char_type str[] = { Cs..., 0 };

public:
    static constexpr auto value = str;

    constexpr auto operator()() const { return value; }

    constexpr operator const char_type * () const
    {
        return value;
    }
};

// create "nullptr" specialization:
// empty string, but returns nullptr instead of pointer to null
template <>
struct kas_string<std::nullptr_t> : kas_string<>
{
    constexpr kas_string() {}
   
    static constexpr char_type *value = nullptr;
    static constexpr std::size_t size = 0;

    constexpr auto operator()() const { return value; }
    
    constexpr operator const char_type * () const
    {
        return value;
    }
};


//
// Define string concatination 
//

namespace detail
{
	template <typename...> struct str_cat_impl;

    // no strings to str_cat
    template <>
    struct str_cat_impl<> : kas_string<> {};

    // 1 string to str_cat
    template <typename T1, T1...Str1>
    struct str_cat_impl<kas_string<T1, Str1...>>
        : kas_string<T1, Str1...> {};

    // 2 strings to str_cat
    template <typename T1, T1...Str1, typename T2, T2...Str2>
    struct str_cat_impl<kas_string<T1, Str1...>, kas_string<T2, Str2...>>
    {
        // XXX fine to convert char to char16_t, but needs to be UTF aware...
        using largest = std::conditional_t<sizeof(T1) >= sizeof(T2), T1, T2>;
        using type    = kas_string<largest, Str1..., Str2...>;
    };

    // 3+ strings to str_cat (combine first 2 & recurse)
    template <typename S1, typename S2, typename...Rest>
    struct str_cat_impl<S1, S2, Rest...>
        : str_cat_impl<meta::_t<str_cat_impl<S1, S2>>, Rest...> {};
}

template <typename...Strs>
using str_cat = meta::_t<detail::str_cat_impl<Strs...>>;

//
// Define string literal conversion function
//

#define KAS_STRING_MAX_STR  16
#define KAS_STRING_GET_C(s)                 \
	::kas::string::detail::getc_n((s), 0),  \
	::kas::string::detail::getc_n((s), 1),  \
	::kas::string::detail::getc_n((s), 2),  \
	::kas::string::detail::getc_n((s), 3),  \
	::kas::string::detail::getc_n((s), 4),  \
	::kas::string::detail::getc_n((s), 5),  \
	::kas::string::detail::getc_n((s), 6),  \
	::kas::string::detail::getc_n((s), 7),  \
	::kas::string::detail::getc_n((s), 8),  \
	::kas::string::detail::getc_n((s), 9),  \
	::kas::string::detail::getc_n((s), 10), \
	::kas::string::detail::getc_n((s), 11), \
	::kas::string::detail::getc_n((s), 12), \
	::kas::string::detail::getc_n((s), 13), \
	::kas::string::detail::getc_n((s), 14), \
	::kas::string::detail::getc_n((s), 15)

#define KAS_STRING(s) ::kas::string::detail::make_string<         \
                  ::kas::string::detail::get_size((s))            \
                , decltype(::kas::string::detail::getc_n((s), 0)) \
                , KAS_STRING_GET_C((s))                           \
                > 

namespace detail
{
	template <typename T, typename, T...Cs>
	struct do_make_string;

    // generate string when input characters exhausted
	template <typename T, T...Str>
	struct do_make_string<T, std::integer_sequence<T, Str...>>
        : kas_string<T, Str...> {};

    // generate string when zero char found
    // NB: can't specialize based on dependent non-type argument
	template <typename T, T...Str, T C0, T...Cs>
	struct do_make_string<T, std::integer_sequence<T, Str...>, C0, Cs...>
    {
        using type = std::conditional_t<
                  C0 != 0
                , meta::_t<do_make_string<T, std::integer_sequence<T, Str..., C0>, Cs...>>
                , kas_string<T, Str...>
                >;
    };

    template <std::size_t N, typename T, T...Cs>
    struct make_string_impl
    {
        static_assert(N <= KAS_STRING_MAX_STR   // trailing null not included in N
                    , "KAS_STRING: string literal too long");

        using type = meta::_t<do_make_string<T, std::integer_sequence<T>, Cs...>>;
    };

	template <std::size_t N, typename T, T...Cs>
	using make_string = meta::_t<make_string_impl<N, T, Cs...>>;

	template <typename T, std::size_t N>
	constexpr T getc_n(const T(&str)[N], std::size_t i)
		{ return i < N ? str[i] : 0; }

	template <typename T, std::size_t N>
	constexpr auto get_size(const T(&str)[N])
		{ return N - 1; } // ignore null

}
}

#endif
