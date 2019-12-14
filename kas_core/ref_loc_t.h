#ifndef KAS_CORE_REF_LOC_T_H
#define KAS_CORE_REF_LOC_T_H

// ref_loc_t
// 
// The `ref_loc_t` is a handle to a `kas_object` derived instance.
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
#include "parser/kas_position.h"
//#include "parser/parser_config.h"

#include <meta/meta.hpp>
#include <boost/type_index.hpp>


namespace kas::core
{

// create position-tagged `index_t` used in `expr` subsystem
// NB: two 32-bit indexes fit in a 64-bit type.

// forward declare `ref_loc` template
template <typename T, typename Index = void> struct ref_loc;

// declare "convience" template: take `template` instead of `type`
template <template <typename> class T, typename Index = void>
using ref_loc_t = ref_loc<meta::quote<T>, Index>;



struct ref_loc_tag {};

template <typename T, typename Index>
struct ref_loc : ref_loc_tag
{
    using index_default = uint32_t;
    using type     = ref_loc;

    using index_t  = meta::if_<std::is_void<Index>, index_default, Index>;

    // XXX doesn't eval obj_t. But doesn't hurt.
    using object_t = meta::_t<meta::id<meta::invoke<T, ref_loc>>>;

    ref_loc() = default;

    // create object_t instance & return "reference"
    template <typename...Ts>
    static ref_loc add(Ts&&...args)
    {
        return object_t::add(std::forward<Ts>(args)...).ref();
    }

    // get `emits_value` from base type
    using emits_value = meta::_t<meta::defer<expression::emits_value, object_t>>;

    // basic tests for `ref`
    //bool empty() const { return !index; }
    operator bool() const { return index; }
    bool operator== (ref_loc const& other) const
        { return index && (index == other.index); }
    
    // XXX modify object call to `get_p(T)` to simplify specialization
    
    // specialize `get_p` for allowed types (eg e_fixed_t, kas_loc)
    template <typename U>
    std::enable_if_t<!std::is_same_v<U, parser::kas_loc>, U const*>
    get_p() const { return nullptr; }

    // use SFINAE to specialize for `kas_loc`
    template <typename U>
    std::enable_if_t<std::is_same_v<U, parser::kas_loc>, U const*>
    get_p() const
    {
        return _loc ? &_loc : nullptr;
    }


    // return *mutable* reference to base type
    // NB: member template defers dependent name resolution until instantiation
    template <typename...Ts>
    object_t& get(Ts&&...) const { return object_t::get(index, loc()); }

    // define getter & setter for `loc`
    parser::kas_loc const& get_loc() const { return loc(); }
    parser::kas_loc const& loc() const { return _loc; }
    ref_loc& set_loc(parser::kas_loc new_loc)
    {
        _loc = new_loc;
        return *this;
    }

    template <typename OS> void print(OS& os) const;

private:
    // declare friend function to access ctor
    friend ref_loc get_ref(object_t const& obj, ref_loc const&)
    {
        return { obj.index(), obj.loc() };
    }

    // private ctor: `friend get_ref()` can use it.
    ref_loc(index_t index, parser::kas_loc loc = {})
        : index(index), _loc(loc) {}

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

template <typename T, typename Index>
template <typename OS>
void ref_loc<T, Index>::print(OS& os) const
{
    os << "XXX ref_loc::print ";
    os << "[" << boost::typeindex::type_id<object_t>().pretty_name();
    auto& l = loc();
    if (l)
        os << ": " << index << " loc: " << l.where();
    os << "]" << std::flush;
}
}
#endif
