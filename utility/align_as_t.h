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
        template <typename Derived_t, typename Align_t>
        struct alignas(Align_t) alignas_t
        {
            using derived_t = Derived_t;
            using align_t   = Align_t;
            using base_t    = alignas_t;

            constexpr alignas_t(align_t data = {})
            {
                void *v = this;
                *static_cast<align_t*>(v) = data;
                _size_ok();
            }

            constexpr auto& assign(align_t data)
            {
                void *v = this;
                *static_cast<align_t *>(v) = data;
                return *static_cast<derived_t *>(v);
            }

            constexpr auto& operator=(align_t data)
            {
                return assign(data);
            }

            constexpr auto value() const
            {
                void const *v = this;
                return *static_cast<align_t const*>(v);
            }

            constexpr static auto& cast(align_t& value)
            {
                void *v = &value;
                return *static_cast<derived_t*>(v);
            }

            constexpr static auto cast(align_t value)
            {
                void *v = &value;
                return *static_cast<derived_t*>(v);
            }
            
            constexpr static auto& cast(align_t *p)
            {
                void *v = p;
                return *static_cast<derived_t*>(v);
            }

            constexpr static auto& cast(align_t const *p)
            {
                void const *v = p;
                return *static_cast<derived_t const*>(v);
            }

        private:
            constexpr void _size_ok();
        };

        template <typename Derived_t, typename Align_t>
        constexpr void alignas_t<Derived_t, Align_t>::_size_ok()
        {
            static_assert(sizeof(*this) == sizeof(Derived_t),
                    "alignas_t sizeof mismatch");
            static_assert(alignof(Derived_t) == alignof(Align_t),
                    "alignas_t alignof mismatch");
        }
    }

}

#endif
