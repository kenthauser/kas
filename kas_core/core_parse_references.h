#ifndef KAS_CORE_CORE_PARSE_REFERENCES_H
#define KAS_CORE_CORE_PARSE_REFERENCES_H

#include "core_types.h"

#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/spirit/home/x3.hpp>

namespace kas::core
{
    template <typename T, typename RT = T>
    struct core_parsed_ref: kas_pos
    {
        using type        = core_parsed_ref;
        using value_type  = T;
        using return_type = RT;

        using index_t     = typename T::index_t;
        using object_t    = typename T::object_t;
        
        core_parsed_ref() = default;
        core_parsed_ref(T const& obj) : ref(obj) {}

        // access parsed string
        auto        begin() const { return first; }
        auto const& end()   const { return last;  }

        // has object (or reference) been assigned?
        bool empty() const { return ref.empty(); }

        // override-able function to get `ref_loc`
        // NB: not `const` because can change value for `empty()`
        return_type get()
        {
            if (empty()) {
                auto& obj = object_t::get(first, last, *this);
                ref       = obj.ref(*this);
            }
            return ref;
        }
        
        // can't specialize `operator` functions.
        // so just return member fn.
        operator return_type()
        {
            return get();
        }
        
    //private:
        T ref;
    };
#if 0
    template <typename T, typename RT>
    auto core_parsed_ref<T, RT>::get() -> return_type
    {
        if (empty()) {
            auto& obj = object_t::get(first, last, *this);
            ref       = obj.ref(*this);
        }
        return ref;
    }
#endif
    using parse_symbol_ref  = core_parsed_ref<symbol_ref>;
    using parse_addr_ref    = core_parsed_ref<addr_ref>;
    using parse_missing_ref = core_parsed_ref<missing_ref, expr_t>;
}

// boost fusion adaptors must be at top namespace level
BOOST_FUSION_ADAPT_STRUCT(kas::core::parse_symbol_ref, ref)
//BOOST_FUSION_ADAPT_STRUCT(kas::core::parse_addr_ref, ref)
//BOOST_FUSION_ADAPT_STRUCT(kas::core::parse_missing_ref, ref)

#endif
