#ifndef KAS_CORE_REF_LOC_T_H
#define KAS_CORE_REF_LOC_T_H

// ref_loc
// 
// The `ref_loc` is a handle to a `kas_object` derived instance.
// The `ref_loc` instance holds the index of the actual instance
// as well as the location where it was parsed. The `ref_loc_t` is
// designed to be used as a handle to types being stored in the expression
// variant. It holds two 32-bit indexes.

// The `ref_loc_t` provides the following functions:
//
// T& get()         -- return mutable instance to wrapped type
// bool empty()     -- true if no instance of T associated with reference
// bool operator==  -- compare two instances (as ref's)
//                     NB: true iff referenced object is same

#include "expr/expr_types.h"
#include "parser/kas_loc.h"

#include <meta/meta.hpp>
#include <boost/type_index.hpp>


namespace kas::core
{

// create position-tagged `index_t` used in `expr` subsystem
// NB: two 32-bit indexes fit in a 64-bit type.

// forward declare `ref_loc` template
template <typename T, typename Index = void, typename = void> struct ref_loc;

// declare "convience" template: take `template` instead of `type`
template <template <typename> class T, typename Index = void>
using ref_loc_tpl = ref_loc<meta::quote<T>, Index>;

// declare "convience" template: take `type` for type
namespace detail
{
    template <typename T>
    struct pass_t
    {
        template <typename...Ts>
        using invoke = T;
    };
}

template <typename T, typename Index = void>
using ref_loc_t = ref_loc<detail::pass_t<T>, Index>;

namespace detail
{
    // support calling object `set_loc` if defined
    template <typename object, typename = void>
    struct has_set_loc : std::false_type {};

    template <typename T>
    struct has_set_loc<T, std::void_t<
            decltype(std::declval<T>().set_loc(parser::kas_loc()))
            >> : std::true_type {};
}

// specialze for `T` is void (ie when floating point not supported)
template <typename T, typename Index>
struct ref_loc<T, Index, meta::invoke<T, void>>
{
    static_assert(std::is_same_v<T, ref_loc>, "Ref_Loc partial");
    using object_type = void;
};


struct ref_loc_tag {};

template <typename T, typename Index, typename>
struct ref_loc : ref_loc_tag
{
    using index_default = uint32_t;
    using type     = ref_loc;

    using index_t  = meta::if_<std::is_void<Index>, index_default, Index>;
    using object_t = meta::_t<meta::id<meta::invoke<T, ref_loc>>>;

    ref_loc() = default;

    ref_loc(object_t const& obj, parser::kas_loc loc = {})
        : index(obj.index()), _loc(loc)
        {
            if (!index)
                throw std::logic_error{"ref_loc::ctor: transient object"};
        }

    // public ctor: create object_t instance & return "reference"
    template <typename...Ts>
    static ref_loc add(Ts&&...args)
    {
        auto& obj = object_t::add(std::forward<Ts>(args)...);
        return obj.ref(obj.loc());
    }

    // get `emits_value` from base type
    using emits_value = meta::_t<meta::defer<expression::emits_value, object_t>>;

    // basic tests for `ref`
    // 1) empty & 2) equivalence
    operator bool() const { return index; }
    bool operator== (ref_loc const& other) const
        { return index && (index == other.index); }

    // getters for object and loc + setter for loc
    // NB: return *mutable* reference to base type
    object_t& get() const { return object_t::get(index); }
    //auto&     loc() const { return _loc; }
    void  set_loc(parser::kas_loc new_loc)
    {  
        _loc = new_loc;

#if 0
        // also set `object` loc if setter is defined (only `core_expr`)
        if constexpr (detail::has_set_loc<object_t>::value)
            get().set_loc(new_loc);
#endif
    }

    // support `expr_variant` get_p/set methods to retrieve/set values
    // entrypoint from `expr_variant`
    //template <typename U>
    //U const *get_p(U, void * = {}) const { return get_p<U>(); }
   
    // actual `get_p` dispactch
    template <typename U>
    U const *get_p() const { return get_p(U{}); }

    template <typename OS> void print(OS& os) const;

    // define getter & setter for `loc`
    parser::kas_loc const& get_loc() const { return _loc; }

private:
    // declare friend function to access ctor
    friend ref_loc get_ref(object_t const& obj, parser::kas_loc const& loc)
    {
        return { obj.index(), loc };
    }

    // private ctor: `friend get_ref()` can use it.
    ref_loc(index_t index, parser::kas_loc loc = {})
        : index(index), _loc(loc) {}

    // specialize `get_p` for allowed types (eg e_fixed_t, kas_loc)
    // NB: template declaration allows additional specifications per-type
    parser::kas_loc const *get_p(parser::kas_loc) const
    {
        return _loc ? &_loc : nullptr;
    }

#if  0
    // NB: `e_fixed_t` defined after this template. So get value via side door
    template <typename U = object_t
            , typename RT = decltype(std::declval<object_t>().get_fixed_p())>
    RT get_p(std::remove_cv_t<std::remove_pointer_t<RT>>) const
    {
        return get().get_fixed_p();
    }
#endif
    // default: return nullptr
    template <typename U>
    auto get_p(U const&) const { return nullptr; }

    template <typename OS>
    friend OS& operator<<(OS& os, ref_loc const& ref)
    {
        ref.print(os);
        return os;
    }

    index_t index{};
    parser::kas_loc _loc {};

    void _() { static_assert(sizeof(ref_loc) <= sizeof(void*)); };
};

// default printer: print type name & instance variables
template <typename T, typename Index, typename V>
template <typename OS>
void ref_loc<T, Index, V>::print(OS& os) const
{
    os << "["  << boost::typeindex::type_id<object_t>().pretty_name();
    os << ": " << index << " loc: " << _loc.get();
    os << "]"  << std::flush;
}
}
#endif
