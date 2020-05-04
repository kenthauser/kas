#ifndef KAS_TARGET_TGT_DATA_INSERTER_H
#define KAS_TARGET_TGT_DATA_INSERTER_H

////////////////////////////////////////////////////////////////////////////////
//
// When the `tgt` opcode & arguments are serialized, a sequence
// of "8/16/32-bit words" and "expressions" are generated
// based on opcode & arguments. The fixed arguments (8-bit, 16-bit or 32-bit)
// are stored in "core::chunks". When an "expression" needs to be stored,
// instead of ending the current "chunk", an "expression" is stored in the
// `core::opcode` expression list, and the current "chunk" remains ready to 
// accept more data. When the current "chunk" fills, the new "chunk" is created
// at the end of the `core::opcode` expression list.
//
// In addition, the 32/64 bit "fixed" storage area of the `core::opcode` area is
// used as backing store for an initial "chunk-like" area.
//
////////////////////////////////////////////////////////////////////////////////
// 
// `tgt_data_inserter` is used to store "words" and "expressions"
// to be intermixed in a space-efficient manner.
//
// `tgt_data_reader` is used to de-serialize these "words" and "expressions"
// provided the corresponding `read`s match the previous `writes` in type & sequence.
//
// `tgt_data_inserter` has the following methods:
//
//  void reserve(unsigned n)        // allocate chunk if `n` words not available in current
//  T* operator()(expr_t&&, mode)
//  T* operator()(int i   , mode)
//
// NB: it is "writers" resposibility to note if `M_SIZE_SWORD` returned nullptr
// so that it can be read back as `M_SIZE_AUTO`.
//
////////////////////////////////////////////////////////////////////////////////
// 
// `tgt_data_reader` has methods to read back data stored with `tgt_data_inserter`
//
//  void reserve(unsigned n)        // next chunk if `n` words not available in current
//
//   expr_t& get_expr()             // gets data stored as `M_SIZE_EXPR`
//   int     get_expr(M_SIZE_*)     // gets data stored as non-expr
//   T*      get_fixed_p(M_SIZE_WORD/LONG)
//                                  // get pointer to mutable data. Advances pointer.
// 
////////////////////////////////////////////////////////////////////////////////


#include "kas_core/opcode.h"
#include "kas_core/core_chunk_inserter.h"
#include "expr/expr_fits.h"

namespace kas::tgt::opc
{
template <typename VALUE_T, typename EMIT_VALUE_T = int32_t>
struct tgt_data_inserter_t
{
    using value_type   = VALUE_T;
    using emit_value_t = EMIT_VALUE_T;
    using signed_t     = std::make_signed_t<value_type>;
    static_assert(std::is_unsigned_v<value_type>);

    // hook into `core_insn` infrastructure
    using data_t   = typename core::opcode::data_t;
    using Inserter = typename data_t::Inserter;
    
    using chunk_inserter_t = typename core::chunk::chunk_inserter_t<Inserter, value_type>;

    // keep a ref to the `data inserter` then init `chunk_inserter`
    tgt_data_inserter_t(data_t& data) : di(data.di()), ci(di)
    {
        n = data.fixed.size<value_type>();
        p = data.fixed.begin<value_type>();
    }

    // make sure `n` bytes available in current "segment" (fixed or chunk)
    // can optionally specify required alignment of data
    void reserve(unsigned bytes, unsigned alignment = {})
    {
        //std::cout << "inserter: reserve: size = " << +bytes << ", align = " << +alignment << std::endl;
        // validate alignment (convert to chunks)
        if (alignment)
        {
            // NB: alignment & value_type are powers-of-two
            alignment /= sizeof(value_type);    // bytes->chunks
            if (alignment == 1)
                alignment = 0;
        }
        
        // align block if required
        if (alignment)
        {
            // if writing in `chunk` area: forward alignment requirement
            if (n == 0)
                ci.align(alignment);
            
            // else writing in `fixed` area: handle alignment directly
            else
            {
                //std::cout << "insert: fixed area alignment: n = " << +n;
                if (n >= alignment)
                {
                    // align `fixed` buffer
                    auto skip = (n - alignment) % alignment;
                    if (skip)
                    {
                        p += skip;
                        n -= skip;
                    }
                } 
                else
                    n = 0;  // done with fixed
                //std::cout << " -> " << +n << std::endl;
            }
        }

        // convert reservation request to chunks
        // NB: bytes of `zero` means start new chunk
        auto chunks = (bytes + sizeof(value_type) - 1)/sizeof(value_type);
       
        // check if room in fixed
        if (n && !chunks)
            n = 0;
        else if (n < chunks)
            n = 0;      // no room
        else if (n == 0)
            ci.reserve(chunks);
    }

    // insert fixed or expression
    value_type* operator()(expr_t const& e, int size = 0)
    {
        //std::cout << "insert_expr: " << e << " size = " << size << std::endl;
        if (size)
            if (auto ip = e.get_fixed_p())
                return (*this)(*ip, size);

        // insert expression after chunk
        *di++ = e;
        return nullptr;
    }

    // insert data via pointer
    value_type* operator()(value_type *code_p, int bytes = 1)
    {
        auto count = (bytes + sizeof(value_type) - 1)/sizeof(value_type);
#if 0
        auto flags = std::cout.flags();
        std::cout << "insert(): " << std::hex;
        for (auto n = 0; n < count; ++n)
            std::cout << +code_p[n] << " ";
        std::cout << "size = " << count << std::endl;
        std::cout.flags(flags);
 #endif
        reserve(count * sizeof(value_type));

        auto p = insert_one(*code_p);
        while (count > sizeof(value_type))
        {
            count -= sizeof(value_type);
            insert_one(*++code_p);
        }
        return p;
    }

    // infer the size & signed/unsigned from T
    template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
    value_type* operator()(T const& t, int size = 0)
    {
        // NB: `signed` irrevelent on store
        if (!size)
            size = sizeof(T);
        return insert_fixed(t, size);
    }

    // insert `value_type` zero
    value_type* operator()()
    {
        return (*this)(value_type());
    }

    // write in chunks as host endian, aligned appropriately
    template <typename T>
    T *write(T const& data = {})
    {
        // make sure room in current chunk
        //std::cout << "insert::write(): size = " << sizeof(T); 
        //std::cout << " alignof = " << alignof(T) << std::endl;
        
        reserve(sizeof(T), alignof(T));
        
        // allocate memory & move into place
        constexpr auto chunks = sizeof(T)/sizeof(value_type);
        void *raw_p = skip(chunks);             // returns base pointer
        std::memcpy(raw_p, &data, sizeof(T));
        return static_cast<T *>(raw_p);         // return pointer
    }

private:
    value_type* insert_fixed(emit_value_t i, int size);
    value_type* insert_one(value_type i);
    value_type* skip(unsigned chunks)
    {
        // dispatch: local/container/error
        if (!n)
            return ci.skip(chunks);
        else if (n >= chunks)
        {
            auto s = p;
            n -= chunks;
            p += chunks;
            return s;
        }
        else
            throw std::logic_error{"tgt_data_inserter::skip no room for data"}; 
    }
    
    Inserter& di;
    chunk_inserter_t ci;
    value_type *p;
    short n;
};

template <typename VALUE_T, typename EMIT_VALUE_T>
auto tgt_data_inserter_t<VALUE_T, EMIT_VALUE_T>::insert_fixed(emit_value_t i, int size) -> value_type *
{
    // recurse to save chunks in big-endian order
    // add support lambda `_save` because c++ loves indirection...
    auto save = [this](int n, unsigned chunks)
    {
        auto _save = [this](int n, unsigned chunks, const auto& _save) -> value_type *
            {
                // if multiple chunks, need to return first. 
                if (--chunks)
                {
                    auto result = _save(n >> sizeof(value_type) * 8, chunks, _save);
                    insert_one(n);
                    return result;
                }
                return insert_one(n);
            };
        
        return _save(n, chunks, _save);
    };

    // Signed irrelevent on store
    if (size < 0)
        size = -size;


    // convert bytes to chunks
    auto chunks = (size + sizeof(value_type) - 1)/sizeof(value_type);

//#define TRACE_TGT_DATA_INSERTER
#ifdef  TRACE_TGT_DATA_INSERTER
    auto flags = std::cout.flags();
    std::cout << "insert_fixed: " << std::hex << i;
    std::cout << " size = " << size;
    std::cout << " chunks = " << chunks << std::endl;
    std::cout.flags(flags);
#endif

    // no data -- return current pointer
    if (chunks == 0)
        return p;
    
    // if one chunk, short-circuit multi-chunk logic
    if (chunks == 1)
        return insert_one(i);

    // if multiple words, store big-endian & return pointer to first chunk
    reserve(chunks);
    return save(i, chunks);
}

template <typename VALUE_T, typename EMIT_VALUE_T>
auto tgt_data_inserter_t<VALUE_T, EMIT_VALUE_T>::insert_one(value_type i) -> value_type *
{
#ifdef  TRACE_TGT_DATA_INSERTER
    auto flags = std::cout.flags();
    std::cout << "insert_one: " << std::hex << i;
    std::cout << ", n = " << n << std::endl;
    std::cout.flags(flags);
#endif

    //std::cout << "insert_one: " << std::hex << +i << std::endl;
    // save one word in fixed area if room
    if (n)
    {
        --n;
        *p++ = i;
        return p - 1;
    }

    // save in chunk expression
    *ci++ = i;
    return ci.last_p();        // return last write location
}

// `tgt_data_reader_t`  deserializes data stored by `tgt_data_inserter_t` above.
template <typename VALUE_T, typename EMIT_VALUE_T = int32_t>
struct tgt_data_reader_t
{
    using value_type   = VALUE_T;
    using emit_value_t = EMIT_VALUE_T;
    using signed_t   = std::make_signed_t<value_type>;
    static_assert(std::is_unsigned_v<value_type>);

    // need iterator to non-const values
    using data_t          = typename core::opcode::data_t;
    using Iter            = typename data_t::Iter;
    using iter_value_type = typename std::iterator_traits<Iter>::value_type;
    static_assert(!std::is_const_v<iter_value_type>);

    using fixed_t        = typename data_t::fixed_t;
    using chunk_reader_t = typename core::chunk::chunk_reader_t<Iter, value_type>;

    tgt_data_reader_t(data_t const& data)
                        : it(data.iter())
                        , chunk_it(core::chunk_reader<value_type>(it, data.cnt))
    {
        n = data.fixed.size<value_type>();
        p = data.fixed.begin<value_type>();
    }

    // make sure `n` aligned bytes available in current "chunk"
    void reserve(unsigned bytes, unsigned alignment = 0)
    {
        //std::cout << "reader: reserve: size = " << +bytes << ", align = " << +alignment << std::endl;
        // validate alignment (convert to chunks)
        if (alignment)
        {
            // NB: alignment & value_type are powers-of-two
            alignment /= sizeof(value_type);    // bytes->chunks
            if (alignment == 1)
                alignment = 0;
        }
        
        // align block if required
        if (alignment)
        {
            // if writing in `chunk` area: forward alignment requirement
            if (n == 0)
                chunk_it.align(alignment);
            
            // else writing in `fixed` area: handle alignment directly
            else
            {
                //std::cout << "reader: fixed area alignment: n = " << +n;
                if (n >= alignment)
                {
                    // align `fixed` buffer
                    auto skip = (n - alignment) % alignment;
                    if (skip)
                    {
                        p += skip;
                        n -= skip;
                    }
                } 
                else
                    n = 0;  // done with fixed
                //std::cout << " -> " << +n << std::endl;
            }
        }

        // convert to chunks
        auto chunks = (bytes + sizeof(value_type) - 1)/sizeof(value_type);

        // reserve in `fixed` or `chunk` space
        if (chunks == 0 && n)
            n = 0;
        else if (n == 0)
            chunk_it.reserve(chunks);
        else if (n < chunks)
            n = 0;
    }

    // copy next value to expression
    auto& get_expr()
    {
        chunk_it.decr_cnt();        // stealing expression
        return *it++;
    }

    // get fixed value
    emit_value_t get_fixed(int size);

    // infer the size & signed/unsigned from T
    template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
    void get(T& t)
    {
        if constexpr (std::is_signed_v<T>)
            t = (*this)(t, -sizeof(T));
        else
            t = (*this)(t, sizeof(T));
    }

    bool empty() const
    {
        return n == 0 && chunk_it.empty();
    }

    // get mutable pointer to chunk data array of words
    // primary use case: get pointer to stored machine-code.
    // NB: assume size is multiple of sizeof(value_type)
    value_type* get_fixed_p(int bytes = sizeof(value_type))
    {
        reserve(bytes);
        return skip(bytes);
    }

    // read arbitrary type from data stream
    template <typename T>
    T *read(T = {})
    {
        // require properly aligned & sized memory block
        // "consume" data & cast pointer to memory block
        reserve(sizeof(T), alignof(T));
        void *raw_p = skip(sizeof(T));
        return static_cast<T *>(raw_p);
    }


private:
    // get next word
    value_type* next_word_p()
    {
        if (n > 0) {
            n--;
            return p++;
        }
        return chunk_it.get_p();
    }

    value_type *skip(unsigned bytes)
    {
        // convert bytes -> chunks
        auto chunks = bytes / sizeof(value_type);

        if (n)
        {
            auto s = p;
            n -= chunks;
            p += chunks;
            return s;
        }

        return chunk_it.get_p(chunks);
    }

    Iter it;
    chunk_reader_t chunk_it;
    unsigned n;
    value_type *p;
};

template <typename VALUE_T, typename EMIT_VALUE_T>
auto tgt_data_reader_t<VALUE_T, EMIT_VALUE_T>::get_fixed(int size) -> emit_value_t
{
    emit_value_t value {};

    if (size == 0)
        return 0;

    // check if signed
    bool is_signed = size < 0;
    if (size < 0)
        size = -size;

    // convert bytes to chunks
    auto chunks = (size + sizeof(value_type) - 1)/sizeof(value_type);
   
    // multiple chunks are stored big-endian
    // sign-extend first word
    reserve(chunks);
    if (is_signed)
        value = static_cast<signed_t>(*next_word_p());
    else
        value = *next_word_p();

    // extend with rest of chunks    
    while(--chunks)
    {
        // if # chunks exceeds size of `value`, return LSBs
        if constexpr (sizeof(value_type)  >= sizeof (value))
            value = 0;
        else
            value <<= sizeof(value_type) * 8;
        value  |= *next_word_p();       // don't sign-extend extension words
    }

    return value;
}
}

#endif
