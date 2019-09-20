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

struct ref_loc_tag {};
template <typename T, typename Index = uint32_t>
struct ref_loc_t : ref_loc_tag
{
    using type     = ref_loc_t;
    using index_t  = Index;
    using object_t = T;

    ref_loc_t() = default;

    // create object_t instance & return "reference"
    template <typename...Ts>
    static auto add(Ts&&...args)
    {
        return T::add(std::forward<Ts>(args)...).ref();
    }

    // get `emits_value` from base type
    using emits_value = meta::_t<meta::defer<expression::emits_value, T>>;

    // basic tests for `ref`
    //bool empty() const { return !index; }
    operator bool() const { return index; }
    bool operator== (ref_loc_t const& other) const
        { return index && (index == other.index); }

    // specialize `get_p` for allowed types (eg e_fixed_t, kas_loc)
    template <typename U>
    std::enable_if_t<!std::is_same<U, parser::kas_loc>::value, U const*>
    get_p() const { return nullptr; }

    // use SFINAE to specialize for `kas_loc`
    template <typename U>
    std::enable_if_t<std::is_same<U, parser::kas_loc>::value, U const*>
    get_p() const
    {
        return loc ? &loc : nullptr;
    }


    // return *mutable* reference to base type
    // NB: member template defers dependent name resolution until instantiation
    template <typename...Ts>
    T& get(Ts&&...) const { return T::get(index, loc); }

    // define getter & setter for `loc`
    parser::kas_loc const& get_loc() const { return loc; }
    ref_loc_t& set_loc(parser::kas_loc new_loc)
    {
        loc = new_loc;
        return *this;
    }

    template <typename OS> void print(OS& os) const;

private:
    // declare friend function to access ctor
    friend ref_loc_t get_ref(T const& obj, ref_loc_t const&)
    {
        return { obj.index(), obj.loc() };
    }

    // private ctor: `friend get_ref()` can use it.
    ref_loc_t(index_t index, parser::kas_loc loc = {})
        : index(index), loc(loc) {}

    template <typename OS>
    friend OS& operator<<(OS& os, ref_loc_t const& ref)
    {
        ref.print(os);
        return os;
    }

    index_t index{};
    parser::kas_loc loc {};

    void _() { static_assert(sizeof(ref_loc_t) <= sizeof(void*)); };
};


template <typename T, typename Index>
template <typename OS>
void ref_loc_t<T, Index>::print(OS& os) const
{
    auto loc_str = loc.where();
    os << "[" << boost::typeindex::type_id<T>().pretty_name();
    os << ": " << index << " loc: " << loc_str << "]";
}

}
#endif
