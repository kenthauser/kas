#ifndef KAS_UTILITY_ALIGNAS_T_H
#define KAS_UTILITY_ALIGNAS_T_H

//
// alignas_t: a CRTP mixin which allows derived class to also
//            be treated as an integral constant. 
//
//            Using the `cast` method, an integral value can be 
//            treated as instance of the derived class.
//
//            Using the `value` method, an instance of the derived
//            class can be converted to integral value
//

namespace kas
{
namespace detail
{
    template <typename Derived_t, typename Align_t, typename...Bases>
    struct alignas(Align_t) alignas_t : Bases...
    {
        using derived_t = Derived_t;
        using value_t   = Align_t;
        using base_t    = alignas_t;

        constexpr alignas_t(value_t data = {})
        {
            void *v = this;
            *static_cast<value_t*>(v) = data;
            _size_ok();
        }

        constexpr auto& assign(value_t data)
        {
            void *v = this;
            *static_cast<value_t *>(v) = data;
            return *static_cast<derived_t *>(v);
        }

        constexpr auto& operator=(value_t data)
        {
            return assign(data);
        }

        constexpr auto value() const
        {
            void const *v = this;
            return *static_cast<value_t const*>(v);
        }

        constexpr static auto& cast(value_t& value)
        {
            void *v = &value;
            return *static_cast<derived_t*>(v);
        }

        constexpr static auto cast(value_t value)
        {
            void *v = &value;
            return *static_cast<derived_t*>(v);
        }
        
        constexpr static auto& cast(value_t *p)
        {
            void *v = p;
            return *static_cast<derived_t*>(v);
        }

        constexpr static auto& cast(value_t const *p)
        {
            void const *v = p;
            return *static_cast<derived_t const*>(v);
        }

    private:
        constexpr void _size_ok();
    };

    template <typename Derived_t, typename Align_t, typename...Bases>
    constexpr void alignas_t<Derived_t, Align_t, Bases...>::_size_ok()
    {
        static_assert(sizeof(*this) == sizeof(Derived_t),
                "alignas_t sizeof mismatch");
        static_assert(alignof(Derived_t) == alignof(Align_t),
                "alignas_t alignof mismatch");
    }
}

template <typename...Ts>
using alignas_t = detail::alignas_t<Ts...>;
}

#endif
