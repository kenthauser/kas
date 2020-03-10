#ifndef KAS_CORE_CHUNK_TYPE_H
#define KAS_CORE_CHUNK_TYPE_H




#include "utility/print_type_name.h"
#include <meta/meta.hpp>
#include <cstdint>

namespace kas { namespace core
{

namespace chunk
{
// declare chunk times
template <size_t CHUNK_PTR_N = 1>
struct _CHUNK_BASE_TYPE { void *v[CHUNK_PTR_N]; };

using CHUNK_BASE_TYPE = _CHUNK_BASE_TYPE<>;
using CHUNK_TYPES     = meta::list<uint8_t, uint16_t, uint32_t>;

template <typename T>
constexpr auto CHUNKS_PER_BLOCK = sizeof(CHUNK_BASE_TYPE)/sizeof(T);

// tag to identify `chunk type`
struct e_chunk_tag {};

// create chunk_t with `N` chunks of type `T`
template <typename T>
struct e_chunk_full : e_chunk_tag
{
    using value_type = T;
    using iter_type  = T*;
    
    auto begin()       { return std::begin(chunks); }
    auto begin() const { return std::begin(chunks); }
    auto end()         { return std::end(chunks); }
    auto end()   const { return std::end(chunks); }

    static auto constexpr count() { return CHUNKS_PER_BLOCK<T>; }
    
    // NB: align/reserve/advance are `static` so that they
    // can be used on `full` or `partial` chunks.

    // true if need new block, otherwise `it` is advanced
    static bool align(unsigned n, iter_type& it, iter_type& bgn)
    {
        auto idx = std::distance(bgn, it);
        auto i = calc_align(n, idx);
        if (i < 0)
            return true;    // need new block
        if (i != 0)
            std::advance(it, i);
        return false;
    }
    
    // true if doesn't fit
    static bool reserve(unsigned n, iter_type& it, iter_type& end)
    {
        auto room = std::distance(it, end);
        return n > room;
    }

    static bool advance(unsigned n, iter_type& it, iter_type& end)
    { 
        auto room = std::distance(it, end);
        std::advance(it, n);
        if (n < room)
            return false;
        if (n == room)
            return true;
        throw std::logic_error{"chunk_t::advance: past end of chunk"};
    }

    template <typename OS> void print(OS& os) const;

protected:
    // chunks to advance to "align" pointer.
    // value < 0: need new block
    static int calc_align(unsigned n, unsigned idx)
    {
        int i = (n - idx) % n;
        if ((i + idx) >= count())
            return -1;      // no room
        return i;
    }

    T chunks[CHUNKS_PER_BLOCK<T>];
};


// create partial chunk block: derive from full chunk type...
template <typename T>
struct e_chunk : e_chunk_full<T>
{
    using full_type = e_chunk_full<T>;
    using full_type::chunks;

private:
    // use last location to hold count of filled chunks
    static constexpr unsigned _chk_idx = full_type::count() - 1;
    auto& index()       { return chunks[_chk_idx]; };
    auto  index() const { return chunks[_chk_idx]; };

public:
    // construct empty chunk
    e_chunk() : full_type() { index() = 0; }

    // access "full chunk" base class
    full_type& full_t()     { return *this; }

    // overwrite interface functions
    // NB: end() is next write location
    using full_type::begin;
    auto end() const   { return begin() + count(); }
    auto end()         { return begin() + count(); }

    auto count() const { return index(); };

    //
    // methods to facilitate writing to chunks
    //
    // NB: routines return `true` if need to be converted to "full" type
    //

    bool add(T const& t)
    {
        auto n = index();
        chunks[n++] = t;

        // if chunk is full, don't overwrite last element with index...
        if (n == full_type::count())
            return true;

        // not full, update accounting
        index() = n;
        return false;
    }

    bool align(unsigned n)
    {
        auto i = full_type::calc_align(n, index());
        if (i < 0)
            return true;        // if no room in current block
        index() += i;
        return false;
    }
    
    bool reserve(unsigned n)
    {
        n += index();
        return n > full_type::count();
    }

    bool advance(unsigned n)
    { 
        index() += n;
        if (index() < full_type::count())
            return false;
        if (index() == full_type::count())
            return true;
        throw std::logic_error{"chunk_t::advance: past end of chunk"};
    }

    template <typename OS> void print(OS& os) const;
};

//
// Declare print methods 
//

template <typename OS, typename T>
void print_chunk(OS& os, T const* p, size_t n = 0)
{
    os << "{";
    while (n--)
    {
        os << std::hex << (unsigned)*p++;
        if (n) os << ", ";
    }
    os << "}";
}

template <typename T>
template <typename OS>
void e_chunk_full<T>::print(OS& os) const
{
    print_chunk(os, begin(), count());
}

template <typename T>
template <typename OS>
void e_chunk<T>::print(OS& os) const
{
    print_chunk(os, begin(), count());
}


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

}}
using chunk::detail::chunk_types;
}}

#endif
