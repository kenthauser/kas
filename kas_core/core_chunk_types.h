#ifndef KAS_CORE_CHUNK_TYPE_H
#define KAS_CORE_CHUNK_TYPE_H

#include <boost/mpl/vector.hpp>
#include <boost/mpl/fold.hpp>
#include <boost/mpl/push_back.hpp>

#include <cstdint>
#include "utility/print_type_name.h"

namespace kas { namespace core
{
    // XXX remove MPL
    //namespace mpl = boost::mpl;
    namespace chunk
    {
        // declare chunk times
        template <size_t CHUNK_PTR_N = 1>
        struct _CHUNK_BASE_TYPE { void *v[CHUNK_PTR_N]; };

        using CHUNK_BASE_TYPE = _CHUNK_BASE_TYPE<>;
        using CHUNK_TYPES     = meta::list<uint8_t, uint16_t, uint32_t>;

        template <typename T>
        constexpr auto CHUNKS_PER_BLOCK = sizeof(CHUNK_BASE_TYPE)/sizeof(T);

        namespace detail {
            template <typename OS, typename T>
            void print_chunk(OS& os, T const* p, size_t n = 0)
            {
                os << "{";
                while (n--) {
                    os << std::hex << (unsigned)*p++;
                    if (n) os << ", ";
                }
                os << "}";
            }
        }

        // tag to identify `chunk type`
        struct e_chunk_tag {};

        // create chunk_t with `N` chunks of type `T`
        template <typename T>
        struct e_chunk_full : e_chunk_tag
        {
            using value_type = T; 

            union
            {
                CHUNK_BASE_TYPE _base;
                T chunks[CHUNKS_PER_BLOCK<T>];
            };

            T const& operator[](size_t n) const { return chunks[n]; }
            T&       operator[](size_t n)       { return chunks[n]; }

            value_type *begin()       { return std::begin(chunks); }
            value_type *end()         { return std::end(chunks); }

            //decltype(auto) begin() const { return std::begin(chunks); }
            //decltype(auto) end()   const { return std::end(chunks); }

            template <typename OS>
            void print(OS& os) const
            {
                detail::print_chunk(os, chunks, CHUNKS_PER_BLOCK<T>);
            }

            template <typename U>
            U const* get_rd_addr(T*& it) const
            {
                return get_aligned_ptr<U>(it, this->end());
            }

        private:
            // get pointer to aligned & allocated storage for type `U`
            // if no such location remains in buffer, return  nullptr
            // otherwise advance `s` past read/write location
            // NB: `U` must be a multiple of `T` (static_assert enforced)
            template <typename U>
            U* get_aligned_ptr(T*& s, T const* e) const
            {
                static_assert(std::is_integral<U>::value);
                static_assert(sizeof(U) >= sizeof(T));
                static_assert((sizeof(U) % sizeof(T)) == 0);
                static_assert(sizeof(U) <= sizeof(chunks));

                size_t room{(e-s) * sizeof(T)};
                void *ptr = s;

                if (!std::align(alignof(U), sizeof(U), ptr, room))
                    return nullptr;

                s = ptr;
                s += sizeof(U)/sizeof(T);

                return ptr;
            }
        };

        // special case U == T
        // template <typename T> template <>
        // inline auto e_chunk_full<T>::get_aligned_ptr<T>(T*& s, T const*) -> T*
        // {
        //     return s++;
        // }

        // create partial chunk block: derive from full chunk type...
        template <typename T>
        struct e_chunk : e_chunk_full<T>
        {
            using full_type = e_chunk_full<T>;
            using full_type::chunks;

            // use last location to hold count of filled chunks
            static constexpr unsigned index = CHUNKS_PER_BLOCK<T> - 1;

            e_chunk() { chunks[index] = 0; }

            bool add(T const& t)
            {
                auto n = chunks[index];
                chunks[n++] = t;

                // if chunk is full, don't overwrite last element with index...
                if (n == CHUNKS_PER_BLOCK<T>)
                    return true;

                // not full, just update accounting update accounting
                chunks[index] = n;
                return false;
            }

            // write needs index, not address, because chunk might need to
            // be re-written as full chunk before data is stored
            template <typename U>
            int get_wr_index(bool& full)
            {
                print_type_name{"wr_index: chunk_t"}.name<T>();
                print_type_name{"wr_index: dest_t"}.name<U>();
                std::cout << "wr_index: index = " << chunks[index] << std::endl;

                auto it = &chunks[chunks[index]];
                if (!this->template get_aligned_ptr<U>(it, full_type::end()))
                    return -1;

                // NB: write happens in return function, so last chunk
                //     is still available as scratch
                full = (it == full_type::end());
                chunks[index] = it - begin();

                std::cout << "wr_index: full: " << full;
                std::cout << " index = " << chunks[index];
                std::cout << " wr_index = " << chunks[index] - sizeof(U)/sizeof(T);
                std::cout << std::endl;

                return chunks[index] - (sizeof(U)/sizeof(T));
            }

            // access full chunk base class
            full_type& full_t() { return *this; }

            template <typename OS>
            void print(OS& os) const
            {
                detail::print_chunk(os, chunks, chunks[index]);
            }

            using full_type::begin;
            decltype(auto) end()        { return begin() + chunks[index]; }
            //decltype(auto) end()  const { return begin() + chunks[index]; }
        };


        namespace detail
        {
            template <typename T>
            using gen_chunk_type = meta::list<e_chunk<T>, e_chunk_full<T>>;

            using chunk_types = meta::join<
                            meta::transform<
                                  CHUNK_TYPES
                                , meta::quote_trait<gen_chunk_type>
                                >
                            >;

        }
    }
    using chunk::detail::chunk_types;
}}

#endif
