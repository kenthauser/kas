#ifndef KAS_TARGET_TGT_DATA_INSERTER_H
#define KAS_TARGET_TGT_DATA_INSERTER_H

////////////////////////////////////////////////////////////////////////////////
//
// When the `tgt` opcode & arguments are serialized, a sequence
// of "16-bit words", "32-bit words", and "expressions" are generated
// based on opcode & arguments. The fixed arguments (either 16-bit or 32-bit)
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
// where `mode` describes information to be stored. Supported values are:
//
//  M_SIZE_EXPR     // store expression. returns nullptr
//
//  M_SIZE_NONE     // no data stored. return pointer to previous word
//  M_SIZE_ZERO
//
//  M_SIZE_LONG     // 32-bit word. Stored big-endian. Return pointer to most-significant word
//  M_SIZE_UWORD    // unsigned 16-bit word. return pointer to word
//  M_SIZE_SWORD:   // signed 16-bit word.
//                         - if fits in 16-bits, return pointer to word
//                         - if doesn't fit, store as expression
//
// NB: it is "writers" resposibility to note if `M_SIZE_SWORD` returned nullptr
// so that it can be read back as `M_SIZE_AUTO`.

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
namespace detail
{
    // opcode fixed data area
    using fixed_t = typename core::opcode::data_t::fixed_t;
    
    template <typename value_type, typename Inserter>
    struct tgt_data_inserter
    {
        static_assert(std::is_unsigned_v<value_type>);
        
        using chunk_inserter_t = typename core::chunk::chunk_inserter_t<Inserter, value_type>;
        using signed_t = std::make_signed_t<value_type>;

        // keep a ref to the `data inserter` then init `chunk_inserter`
        tgt_data_inserter(Inserter& di, fixed_t& fixed) : di(di), ci(di)
        {
            n = fixed.size<value_type>();
            p = fixed.begin<value_type>();
        }

        // make sure `n` bytes available in current "segment"
        void reserve(unsigned bytes)
        {
            // convert to chunks
            auto chunks = (bytes + sizeof(value_type) - 1)/sizeof(value_type);
            
            if (n == 0)
                ci.reserve(chunks);
            else if (n < chunks)
                n = 0;
        }

        // insert fixed or expression
        value_type* operator()(expr_t&&e, unsigned size = 0)
        {
            if (size)
                if (auto ip = e.get_fixed_p())
                    return (*this)(*ip, size);

            // insert expression after chunk
            *di++ = std::move(e);
            return nullptr;
        }

        // insert data via pointer: NB size must be > 0
        value_type* operator()(value_type *code_p, unsigned size)
        {
            reserve(size);

            auto p = insert_one(*code_p);
            while (size > sizeof(value_type))
            {
                size -= sizeof(value_type);
                insert_one(*++code_p);
            }
            return p;
        }

        // infer the size & signed/unsigned from T
        template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
        value_type* operator()(T const& t, unsigned size = sizeof(T))
        {
            if constexpr (std::is_signed_v<T>)
                return insert_fixed(t, -size);
            else
                return insert_fixed(t, size);
        }

    private:
        value_type* insert_fixed(int i, int size);
        value_type* insert_one(value_type i);
        
        Inserter& di;
        chunk_inserter_t ci;
        value_type *p;
        short n;
    };

    template <typename value_type, typename Inserter>
    value_type *tgt_data_inserter<value_type, Inserter>::insert_fixed(int i, int size) 
    {
        // Signed irrelevent on store
        if (size < 0)
            size = -size;

        // convert bytes to chunks
        auto chunks = (size + sizeof(value_type) - 1)/sizeof(value_type);

        // no data -- return current pointer
        if (chunks == 0)
            return p;

        // if multiple words, store big-endian & return pointer to first chunk
        if (chunks > 1)
        {
            reserve(chunks);
            
            // store first word & save pointer
            auto first_word  = i >> (--chunks * sizeof(value_type) * 8);
            auto first_p     = (*this)(first_word, sizeof(value_type));
            /* store rest */   (*this)(i, size - sizeof(value_type));
            
            return first_p;
        }
        
        return insert_one(i);
    }

    template <typename value_type, typename Inserter>
    value_type *tgt_data_inserter<value_type, Inserter>::insert_one(value_type i)
    {
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
    template <typename value_type, typename Iter>
    struct tgt_data_reader 
    {
        static_assert(std::is_unsigned_v<value_type>);
        
        // need iterator to non-const values
        using iter_value_type = typename std::iterator_traits<Iter>::value_type;
        static_assert(!std::is_const_v<iter_value_type>);
    
        using fixed_t        = typename core::opcode::data_t::fixed_t;
        using chunk_reader_t = typename core::chunk::chunk_reader_t<Iter, value_type>;
        using signed_t       = std::make_signed_t<value_type>;

        tgt_data_reader(Iter& it, fixed_t& fixed, int cnt)
                            : it(it), chunk_it(core::chunk_reader<value_type>(it, cnt))
        {
            n = fixed.size<value_type>();
            p = fixed.begin<value_type>();
        }

        // make sure `n` words available in current "chunk"
        void reserve(unsigned bytes)
        {
            // convert to chunks
            auto chunks = (bytes + sizeof(value_type) - 1)/sizeof(value_type);
            if (n == 0)
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
        expression::e_fixed_t get_fixed(int size);

        // infer the size & signed/unsigned from T
        template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
        void get(T& t)
        {
            if constexpr (std::is_signed_v<T>)
                t = (*this)(t, -sizeof(T));
            else
                t = (*this)(t, sizeof(T));
        }


        // get *mutable* pointer to chunk data
        // used to get pointer to stored machine-code.
        // NB: assume size is positive multiple of sizeof(value_type)
        value_type* get_fixed_p(int size)
        {
            auto chunks = size/sizeof(value_type);
            
            reserve(chunks);
            auto p = next_word_p();

            // read & discard additional words
            while(--chunks)
                next_word_p();

            return p;
        }

        bool empty() const
        {
            return n == 0 && chunk_it.empty();
        }


    private:
        // get next word
        value_type*  next_word_p()
        {
            if (n > 0) {
                n--;
                return p++;
            }
            return chunk_it.get_p();
        }

        Iter& it;
        chunk_reader_t chunk_it;
        unsigned n;
        value_type *p;
    };

    template <typename value_type, typename Iter>
    expression::e_fixed_t tgt_data_reader<value_type, Iter>::get_fixed(int size)
    {
        expression::e_fixed_t value {};

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
            value <<= sizeof(value_type) * 8;
            value  |= *next_word_p();       // don't sign-extend extension words
        }

        return value;
    }
}

// class template agument deduction doesn't support partial deduction. 
// Use forwarding functions.
template <typename value_type, typename Inserter, typename...Ts>
auto tgt_data_inserter(Inserter& inserter, Ts&&...args)
{
    return detail::tgt_data_inserter<value_type, Inserter>(inserter, std::forward<Ts>(args)...);
}

template <typename value_type, typename Iter, typename...Ts>
auto tgt_data_reader(Iter& iter, Ts&&...args)
{
    return detail::tgt_data_reader<value_type, Iter>(iter, std::forward<Ts>(args)...);
}


}
#endif
