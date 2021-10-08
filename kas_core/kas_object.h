#ifndef KAS_CORE_KAS_OBJECT_H
#define KAS_CORE_KAS_OBJECT_H

//
// CRTP mixin for creating objects used in KAS
//

#include "ref_loc_t.h"
#include "kas_clear.h"

#include <deque>
#include <new>
#include <cassert>
#include <iomanip>
#include <boost/type_index.hpp>

#include "utility/print_type_name.h"

namespace kas::core
{
    namespace detail
    {
        using namespace meta;

        // retrieve `index_t` member type from Ref (default `uint32_t`)
        using default_index_t = uint32_t;
        template <typename T, typename = void>
        struct get_index_t : id<default_index_t> {};

        template <typename T>
        struct get_index_t<T, std::void_t<typename T::index_t>>
                : id<typename T::index_t> {};
    }

struct kas_object_tag {};

template <typename Derived, typename Ref = void
        , template <typename T> class Allocator = std::allocator>
struct kas_object : kas_object_tag
{
    using base_t      = kas_object;
    using derived_t   = Derived;
    using ref_loc_t   = Ref;
    using index_t     = meta::_t<detail::get_index_t<Ref>>;
    using object_t    = derived_t;

    using type        = derived_t;
    
    // for `expr_fits`
    using emits_value = std::true_type;     // object holds value

    // prevent copying or moving base_t instance
    kas_object(kas_object const&)            = delete;
    kas_object(kas_object&&)                 = delete;
    kas_object& operator=(kas_object const&) = delete;
    kas_object& operator=(kas_object&&)      = delete;

    // CRTP casts
    auto& derived() const
        { return *static_cast<derived_t const*>(this); }
    auto& derived()
        { return *static_cast<derived_t*>(this); }


protected:
    static auto& obstack()
    {
        static auto deque_ = new std::deque<Derived, Allocator<Derived>>;
        return *deque_;
    }

public:
    // NB: index not inited. Use `add` to create permanent object
    kas_object(parser::kas_loc loc = {}) : obj_loc(loc) {}

    // create instance (in deque)
    template <typename...Ts>
    static derived_t& add(Ts&&...args)
    {
        auto& s     = obstack();
        auto& t     = s.emplace_back(std::forward<Ts>(args)...);
        t.obj_index = s.size();
        return t;
    }
    
    // lookup instance (using index)
    static derived_t& get(index_t n)
    {
        if (n == 0) {
            std::string name = boost::typeindex::type_id<derived_t>().pretty_name();
            throw std::logic_error(name + ": access object zero");
        }
        return obstack()[n - 1];
    }

    // getters for instance variables
    auto index() const { return obj_index; }
    auto& loc()  const { return obj_loc;   }

    // setter for loc: not normally used
    // special for `core_expr`
    void set_obj_loc (parser::kas_loc const& loc)
    {
        obj_loc = loc;
    }

    // use friend function to create reference
    // NB: only define if `Ref` type defined as non-void
    template <typename U = Ref, typename = std::enable_if_t<!std::is_same_v<U, void>>>
    Ref ref(parser::kas_loc loc = {}) const
    { 
        return get_ref(derived(), loc);
    } 

    static auto num_objects()
    {
        return obstack().size();
    }

protected:
    //
    // Utility methods for `kas_object` objects
    //
    // These are `protected` and must be exposed to be used by
    // derived objects
    // (via for example: `using base_t::for_each;`)
    //

    // object iterators (NB: not normally exposed)
    static auto begin() { return obstack().begin(); }
    static auto end()   { return obstack().end();   }

    // `for_each` variants

    // method to walk thru and modify the `derived_t` table in alloc order
    // `pred` returns true when passed object if `fn` should be applied
    // NB: a mutable reference to `derived_t` is passed to `fn`
    template <typename FN, typename Predicate>
    static void for_each_if(FN fn, Predicate pred)
    {
        for (auto& v : obstack()) {
            if (pred(v))
                fn(v);
        }
    }

    template <typename FN>
    static void for_each(FN fn)
    {
        return for_each_if(fn, [](auto&&) { return true; });
    }

    // method to perform `for_each` over a subset of objects
    template <typename FN>
    static void for_each(FN fn, index_t first, index_t cnt = -1)
    {
        auto obj_it  = begin();
        auto obj_end = end();
        std::advance(obj_it, first - 1);
        while(cnt-- && obj_it != obj_end)
            fn(*obj_it++);
    }

    // method to perform `for_each` over a subset of objects
    template <typename FN>
    static void for_each(FN fn, derived_t const *first_p, index_t cnt = -1)
    {
        for_each(fn, first_p->obj_index, cnt);
    }

    template <typename OS>
    static void dump(OS& os)
    {
        os << "dump: " << derived_t::NAME::value;
        for (auto const& s : obstack()) {
            // print index
            os << std::endl;
            os << std::dec << std::right;
            os << std::setw(4) << s.obj_index << ": ";
            
            s.dump_one(os);
        }
        os << '\n' << std::endl;
    }


    // default dump: just ostream instance
    template <typename OS>
    void dump_one(OS& os) const
    {
        derived().print(os);
    }

//protected:
public:
    // test fixture support: deallocate all instances
    static void clear() {}      // default if `derived_t::clear()` not defined
    static void obj_clear()
    {
        //print_type_name{"kas_object: clear"}.name<derived_t>();
        derived_t::clear();
        obstack().clear();
    }

    static inline kas_clear _c{obj_clear};
private:
    index_t         obj_index {};       // index into obstack (+1)
    parser::kas_loc obj_loc;            // location where first seen
};

}
#endif
