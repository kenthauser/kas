#ifndef KAS_INIT_FROM_LIST_H
#define KAS_INIT_FROM_LIST_H   

// `init_from_list` is a metafunction used to generate a 
// `constexpr std::array` of definitions from a `meta::list` of definitions.
// The resulting type has two member vaiables:
//
//  ::value     a `T const *` pointer to first element of array
//  ::size      `std::size_t` count of elements.
//
// There are two call formats for metafunction:
//
//  1. init_from_list<T, LIST>
//  2. init_from_list<T, LIST, CTOR, CTOR_ARG>
//
// The first format generates `init` type as described above.
// The second format can be used with `VT_CTOR` (defined below) to generate a
// "pointer-to-static-instantiated-instance" array for lists of virtual types.
//
// The second format transforms the LIST using CTOR<CTOR_ARG> before
// generating definitions. Usage patterns for this format include replacing
// members of the `LIST` with indexes into `TYPE_LIST` passed as CTOR_ARG.
// NB: CTOR_ARG defaults to `void`
//
// This header also defines `VT_CTOR`. This `TYPE` allows virtual-base-types
// to be initializes as well. Since derived types can't be stored in an
// `std::array<BASE_TYPE, N>`, `VT_CTOR` is a wrapper-type which instantiates
// the derived type & returns a pointer to the base-type.
//
// When `VT_CTOR` is used, `CTOR_ARG` can be a `meta::list<DEFAULT>` which
// can specify type to instantiate when type of `void` is specified.


#include <meta/meta.hpp>
#include <type_traits>
#include <array>

namespace kas
{
namespace detail
{
    // support routine: `value_of` is `T::value` iff exists
    template <typename T, typename = void>
    struct has_value : std::false_type {};

    template <typename T>
    struct has_value<T, std::void_t<decltype(T::value)>> : std::true_type {};

    template <typename T>
    constexpr auto value_of(T t)
    {
        // if T has `value` member, return that
        if constexpr (has_value<T>::value)
            return T::value;
        else
            return t;
    }
}

// base template: for when `CTOR` specified
// NB: consume `CTOR` & inherit from no-ctor specialization
template <typename T, typename LIST, typename CTOR = void, typename CTOR_ARG = void>
struct init_from_list : 
       init_from_list<T, meta::transform<LIST, meta::invoke<CTOR, CTOR_ARG>>>
       {};

// specialize for when `CTOR` is void
template <typename T, typename...Ts, typename CTOR_ARG>
struct init_from_list<T, meta::list<Ts...>, void, CTOR_ARG>
{
    using type = init_from_list;        // for lazy evaluation

    // NB: disallow zero length arrays...
    static constexpr auto size = sizeof...(Ts);
    static constexpr auto _size = std::max<unsigned>(1, size);
    static constexpr T data[_size] { detail::value_of(Ts())... };
    static constexpr T const* value = data;
};
   

//////////////////////////////////////////////////////////////////
//
// Declare `CTOR` for virtual types.
// Instantiate a derived-type instance & use pointer to init array
//
//////////////////////////////////////////////////////////////////

namespace detail
{
    // NB: This special developed for `tgt::val_t` which passes `list<NAME, T, args...>`.
    // Also used for instantiation of other virtual types, such as `kbfd relocation ops`
    template <typename DFLT, typename T, typename = void>
    struct vt_ctor_impl : vt_ctor_impl<DFLT, meta::list<void, T>> {};

    template <typename DFLT, typename NAME, typename T, typename...Ts>
    struct vt_ctor_impl<DFLT, meta::list<NAME, T, Ts...>, void>
    {
        using type = vt_ctor_impl;
        static const inline T instance{Ts::value...};
        static constexpr auto value = &instance;
    };

    // if default type specified, use it to replace "void" base-type
    template <typename DFLT>
    struct vt_ctor_impl<meta::list<DFLT>, void, void> 
                    : vt_ctor_impl<meta::list<>, DFLT> {};

    // if no default type specified, initialize as nullptr
    template <>
    struct vt_ctor_impl<void, void, void>
    {
        using type = vt_ctor_impl;
        static constexpr std::nullptr_t value{};
    };
}

// since virutal base-class can't be default constructed, allow
// "default" type to be specified
struct VT_CTOR
{
    using type = VT_CTOR;

    template <typename DFLT>
    struct do_vt_ctor
    {
        using type = do_vt_ctor;

        template <typename DEFN>
        using invoke = meta::_t<detail::vt_ctor_impl<DFLT, DEFN>>;
    };

    template <typename DFLT = meta::list<>>
    using invoke = do_vt_ctor<DFLT>;
};

}

#endif
