#ifndef KAS_Z80_DATA_INSERTER_H
#define KAS_Z80_DATA_INSERTER_H

////////////////////////////////////////////////////////////////////////////////
//
// When the `z80` opcode & arguments are serialized, a sequence
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
// `z80_data_inserter` is used to store "words" and "expressions"
// to be intermixed in a space-efficient manner.
//
// `z80_data_reader` is used to de-serialize these "words" and "expressions"
// provided the corresponding `read`s match the previous `writes` in type & sequence.
//
// `z80_data_inserter` has the following methods:
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
// `z80_data_reader` has methods to read back data stored with `z80_data_inserter`
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




namespace kas { namespace z80 { namespace opc
{

// from m68k_extension_t
enum {
      M_SIZE_NONE  = 0
    , M_SIZE_ZERO  = 1
    , M_SIZE_WORD  = 2
    , M_SIZE_LONG  = 3
    , M_SIZE_AUTO  = 8
    , M_SIZE_POST_INDEX = 4
    , M_SIZE_BYTE  = 9
    , M_SIZE_UWORD = 10     // special for serializer
    , M_SIZE_INDEX = 11     // special for serializer
    , M_SIZE_SWORD = 12     // special for serializer
};


namespace detail {
    using z80_op_size_t = uint16_t;
    using fixed_t = typename core::opcode::fixed_t;
    // using namespace core::chunk;

    template <typename Inserter, typename T = z80_op_size_t>
    struct z80_data_inserter_t
    {
        using value_type = T;
        using chunk_inserter_t = typename core::chunk::chunk_inserter_t<Inserter, T>;
        using signed_t = std::make_signed_t<T>;

        // keep a ref to the `data inserter` then init `chunk_inserter`
        z80_data_inserter_t(Inserter& di, fixed_t &fixed)
            : di(di), ci(di)
        {
            n = std::end(fixed.fixed_p) - std::begin(fixed.fixed_p);
            p = std::begin(fixed.fixed_p);
        }

        // make sure `n` words available in current "segment"
        void reserve(std::size_t cnt)
        {
            if (n == 0)
                ci.reserve(cnt);
            else if (n < cnt)
                n = 0;
        }

        T* operator()(expr_t&&e, int mode)
        {
            if (mode != M_SIZE_AUTO)
                if (auto ip = e.get_fixed_p())
                    return (*this)(*ip, mode);

            // insert expression after chunk
            *di++ = std::move(e);
            return nullptr;
        }

        T* operator()(int i, int mode)
        {
            using expression::expr_fits;
            using expression::fits_result;

            switch (mode) {
                default:
                case M_SIZE_NONE:
                case M_SIZE_ZERO:
                    return p;
                case M_SIZE_LONG:
                    {
                        // z80: store big-endian
                        reserve(2);
                        auto first_p  = (*this)(i >> 16   , M_SIZE_UWORD);
                        /* store only */(*this)(i & 0xffff, M_SIZE_UWORD);
                        return first_p;
                    }
                case M_SIZE_SWORD:
                    if (expr_fits{}.fits<signed_t>(i) != fits_result::DOES_FIT)
                        return (*this)(i, M_SIZE_AUTO);
                    // FALLSTHRU
                case M_SIZE_UWORD:
                case M_SIZE_WORD:
                    if (n) {
                        n--;
                        *p++ = i;
                        return p - 1;
                    }
                    *ci++ = i;
                    return ci.last_p();        // return last write location
            }
        }

    private:
        Inserter& di;
        chunk_inserter_t ci;
        T *p;
        short n;
    };

    // `z80_data_reader_t`  deserializes data stored by `z80_data_inserter_t` above.

    template <typename Iter, typename T = z80_op_size_t>
    struct z80_data_reader_t {
        using chunk_reader_t = typename core::chunk::chunk_reader_t<Iter, T>;
        using signed_t = std::make_signed_t<T>;
        using unsigned_t = std::make_unsigned_t<T>;

        z80_data_reader_t(Iter& it, fixed_t& fixed, int cnt)
            : it(it), chunk_it(core::chunk_reader<T>(it, cnt))
        {
            n = std::end(fixed.fixed_p) - std::begin(fixed.fixed_p);
            p = fixed.fixed_p;
        }

        // make sure `n` words available in current "chunk"
        void reserve(std::size_t cnt)
        {
            if (n == 0)
                chunk_it.reserve(cnt);
            else if (n < cnt)
                n = 0;
        }

        // get "expression" variant
        expr_t get_expr(int mode)
        {
            if (mode != M_SIZE_AUTO)
                return get_fixed(mode);
            chunk_it.decr_cnt();        // stealing expression
            return *it++;
        }

        // can return reference with M_SIZE_AUTO
        expr_t& get_expr()
        {
            chunk_it.decr_cnt();        // stealing expression
            return *it++;
        }

        // get fixed value
        expression::e_fixed_t get_fixed(int mode)
        {
            expression::e_fixed_t value;

            switch (mode) {
                case M_SIZE_NONE:
                case M_SIZE_ZERO:
                    return 0;
                case M_SIZE_WORD:
                    return *next_word_p();
                case M_SIZE_SWORD:
                    return static_cast<signed_t>(*next_word_p());
                case M_SIZE_UWORD:
                    return static_cast<unsigned_t>(*next_word_p());
                case M_SIZE_LONG:
                    // stored big-endian
                    reserve(2);     // require adjacent words
                    value  = *next_word_p() << 16;
                    value |= *next_word_p()  & 0xffff;
                    return value;

                case M_SIZE_AUTO:
                case M_SIZE_BYTE:
                default:
                    // need BAD_CASE macro
                    throw std::logic_error("invalid z80_arg deserialize size");
            }
        }

        // get pointer to chunk data
        // NB: allow M_SIZE_WORD or M_SIZE_LONG
        T* get_fixed_p(int mode = M_SIZE_WORD)
        {
            if (mode == M_SIZE_LONG)
                reserve(2);
                
            auto p = next_word_p();
            if (mode == M_SIZE_LONG)
                next_word_p();      // skip next word
            else if (mode != M_SIZE_WORD)
                    throw std::logic_error("invalid z80_arg deserialize size");
            return p;
        }

        bool empty() const
        {
            return n == 0 && chunk_it.empty();
        }


    private:
        // get next word
        T*  next_word_p()
        {
            if (n > 0) {
                n--;
                return p++;
            }
            return chunk_it.get_p();
        }

        Iter& it;
        chunk_reader_t chunk_it;
        std::size_t n;
        T *p;
    };
}

template <typename Inserter>
auto z80_data_inserter(Inserter& it, detail::fixed_t& fixed)
{
    return detail::z80_data_inserter_t<Inserter>(it, fixed);
}

template <typename Iter>
auto z80_data_reader(Iter& it, detail::fixed_t& fixed, int cnt)
{
    return detail::z80_data_reader_t<Iter>(it, fixed, cnt);
}

}}}
#endif
