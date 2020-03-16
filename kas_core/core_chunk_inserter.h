#ifndef KAS_CORE_CHUNK_INSERTER_H
#define KAS_CORE_CHUNK_INSERTER_H

#include "core_chunk_types.h"
#include "kas_core/insn_data.h"

namespace kas::core::chunk
{
using expr_fits = expression::expr_fits;
using fixed_t   = typename opcode::data_t::fixed_t;

namespace detail
{
    using namespace meta;
    using namespace meta::placeholders;

    // return meta::list<> starting with first type of at least sizeof(N)
    template <typename T>
    using T2T_IMPL = find_if<
                        CHUNK_TYPES
                      , lambda<
                            _a
                          , lazy::greater_equal<
                                  lazy::sizeof_<_a>
                                , sizeof_<T>
                                >
                          >
                      >;
}

// return meta::list<> starting with first type of at least sizeof(N)
template <typename T>
using T2CHUNK_T = detail::T2T_IMPL<T>;


template <typename Inserter, typename T, typename CTs = T2CHUNK_T<T>>
struct chunk_inserter_t;

// T compatible with a e_chunk<T>
template <typename Inserter, typename T, typename CHUNK_VT, typename...XT>
struct chunk_inserter_t<Inserter, T, meta::list<CHUNK_VT, XT...>>
{
    static_assert(meta::in<CHUNK_TYPES, CHUNK_VT>::value);

    using value_type = T;
    using data_type  = CHUNK_VT;
    using chunk_type = e_chunk<CHUNK_VT>;

    // hold underlying `expr` inserter
    explicit chunk_inserter_t(Inserter& di) : di(di) {}

    // write expression as integral value or expression
    auto& operator= (expr_t const& e)
    {
        if (auto p = e.get_fixed_p())
            *this = *p;
        else
            add_expr(e);
        return *this;
    }

    // write integral value if `fits`, else as expression
    auto operator= (expression::e_fixed_t e)
    {
        // XXX is expr_fits test correct?
        // expr_fits(int) is constexpr
        if (expr_fits{}.ufits<value_type>(e) == expr_fits::yes)
            add_element(e);
        else
            add_expr(e);
        return *this;
    }

    // make sure at least `n` spots remain in current `chunk`
    // reserve of zero means skip to beginning of chunk
    void reserve(unsigned n)
    {
        // end chunk if not enough room
        auto& chunk = get_chunk();

        if (!n && !chunk.empty())
            flush();
        else if (chunk.reserve(n))
            flush();
    }

    // make sure properly aligned for `n` chunk type
    void align(unsigned n)
    {
        auto& chunk = get_chunk();
        if (chunk.align(n))
            flush();
    }

    // allocate memory for `n` chunks in current chunk
    value_type *skip(unsigned n)
    {
        // record address to write to
        auto& chunk  = get_chunk();
        last_write_p = chunk.end();
        if (chunk.advance(n))
            mark_as_full();     // if writing fills chunk
        return last_write_p;
    }
    
    // iterator extension to obtain location of last write
    auto last_p() const { return last_write_p; }

    // iterator dummy operations
    auto& operator*()     { return *this; }
    auto& operator++()    { return *this; }
    auto& operator++(int) { return *this; }


private:
    // add element to chunk. flush if full.
    void add_element (value_type t)
    {
        auto& chunk = get_chunk();
        last_write_p = chunk.end();     // save address for `last_p()`
        if (chunk.add(t))
            mark_as_full();
    }

    // element incompatible with chunk. flush & add expr_t to underlying queue
    void add_expr (expr_t const& e)
    {
        flush();
        *di++ = e;
    }

    // return reference to (open) chunk to write data
    auto& get_chunk()
    {
        // need (local) iterator extension which allows `last()`
        // to return reference to last write. True for `forward iterators`
        // and extension to inserter at `opc::opc_back_insert_iterator`
        static_assert(std::is_same<
                          decltype(di.last())
                        , typename std::iterator_traits<Inserter>::reference
                        >::value
                    , "required (local) inserter extension not present");

        if (!chunk_p)
        {
            // insert empty chunk & remember where
            *di++    = chunk_type{};
            expr_p   = &di.last();
            chunk_p  = &boost::get<chunk_type>(*expr_p);
        }

        // access variant data directly
        return *chunk_p;
    }

    // current chunk is full -- close it
    void mark_as_full()
    {
        // convert boost::variant<type> to a `full` type
        // NB: should just change variant `tag`, not copy data
        *expr_p = std::move(chunk_p->full_t());
        chunk_p = {};       // flag no chunk
    }

    // done writing to not-filled block
    void flush()
    {
        chunk_p = {};
    }
    
    chunk_type *chunk_p {};
    expr_t     *expr_p;         // pointer to chunk-as-expr
    data_type  *last_write_p;   // last written-to address in chunk
    Inserter&   di;
};

// T too big for any chunk_t
template <typename Inserter, typename T>
struct chunk_inserter_t<Inserter, T, meta::list<>> : Inserter
{
    using value_type = T;
    using data_type  = void;
    explicit chunk_inserter_t(Inserter& di) : Inserter(di) {}
};

namespace detail
{
    // actual `chunk_reader_t` in detail namespace

    template <typename Iter, typename VT, typename CHUNK_VT, typename CHUNK_T>
    struct _chunk_reader_t
    {
        using value_type = VT;
        using data_type  = CHUNK_VT;
        using chunk_type = CHUNK_T;
        using full_type  = typename chunk_type::full_type;


        _chunk_reader_t(Iter& it, std::size_t cnt)
            : it(it), cnt(cnt) {}

        bool empty() const
        {
            return cnt == 0 && chunk_p == chunk_end;
        }

        // used when sharing expression pool with serializer
        void decr_cnt()
        {
            --cnt;
        }

        expr_t& get_expr()
        {
            if (is_chunk())
                throw std::runtime_error("chunk_reader_t: get_expr() in block");
            --cnt;
            return *it++;
        }

        value_type get()
        {
            if (!is_chunk())
                throw std::runtime_error("chunk_reader_t: get() not in block");
            return *chunk_p++;
        }

        value_type* get_p(unsigned n = 1)   // get pointer to `n` chunks
        {
            if (!is_chunk())
                throw std::runtime_error("chunk_reader_t: get_p() not in block");
            auto p = chunk_p;
            chunk_p += n;
            return p;
        }
    
        // make sure at least `n` spots remain in current `chunk`
        // `n` == 0 means start new chunk
        void reserve(unsigned n)
        {
            if (!is_chunk())
                throw std::runtime_error("chunk_reader_t: reserve() not in block");

            // if not at beginning chunk, open new one
            if (!n && chunk_p != chunk_begin)
                chunk_p = chunk_end;

            if (full_type::reserve(n, chunk_p, chunk_end))
                chunk_p = chunk_end;        // if no room, flag chunk empty
        }

        // align iterator for host-format type
        void align(unsigned n)
        {
            if (!is_chunk())
                throw std::runtime_error("chunk_reader_t: align() not in block");

            if (full_type::align(n, chunk_p, chunk_begin))
                chunk_p = chunk_end;        // if no room, flag chunk empty
        }


        template <typename CT = value_type>
        std::enable_if_t<!std::is_void<CT>::value, bool>
        is_chunk ()
        {
            if (chunk_p == chunk_end)
            {
                if (empty())
                    return false;

                // peek at expression queue & see if a `chunk` type
                if (auto p = it->template get_p<chunk_type>())
                {
                    chunk_begin = chunk_p = p->begin();
                    chunk_end   = p->end();
                    --cnt;
                    ++it;
                }
                // ...or a full-chunk type
                else if (auto p = it->template get_p<full_type>())
                {
                    chunk_begin = chunk_p = p->begin();
                    chunk_end   = p->end();
                    --cnt;
                    ++it;
                }
            }
            return chunk_p != chunk_end;
        }


        template <typename  CT = value_type>
        std::enable_if_t<std::is_void<CT>::value, bool>
        is_chunk ()
        {
            return false;
        }

        CHUNK_VT *chunk_p {};
        CHUNK_VT *chunk_begin {};
        CHUNK_VT *chunk_end {};

        Iter& it;
        std::size_t cnt;
    };
}

// where `T` is type held by chunk with underlying type `T2CHUNK_T`
template <typename Iter, typename T, typename CTs = T2CHUNK_T<T>>
struct chunk_reader_t;

template <typename Iter, typename T, typename CHUNK_VT, typename...XT>
struct chunk_reader_t<Iter, T, meta::list<CHUNK_VT, XT...>>
    : detail::_chunk_reader_t<Iter, T, CHUNK_VT, e_chunk<CHUNK_VT>>
{
    static_assert(meta::in<CHUNK_TYPES, CHUNK_VT>::value);
    using reader_t = detail::_chunk_reader_t<Iter, T, CHUNK_VT, e_chunk<CHUNK_VT>>;
    using reader_t::reader_t;
};

template <typename Iter, typename T>
struct chunk_reader_t<Iter, T, meta::list<>>
{
    using value_type = T;

    chunk_reader_t(Iter& it, std::size_t cnt) : it(it), cnt(cnt) {}

    bool empty() const
    {
        return cnt == 0;
    }

    expr_t& get_expr()
    {
        if (is_chunk())
            throw std::runtime_error("chunk_reader_t: e in block");
        --cnt;
        return *it++;
    }

    value_type get()
    {
        if (!is_chunk())
            throw std::runtime_error("chunk_reader_t: not in block");
        return {};
    }

    value_type* get_p()
    {
        if (!is_chunk())
            throw std::runtime_error("chunk_reader_t: not in block");
        return {};
    }

    bool is_chunk () const
    {
        return false;
    }

    Iter& it;
    std::size_t cnt;
};
}

namespace kas::core
{

template <typename T, typename Inserter>
inline auto chunk_inserter(Inserter& di)
{
    using namespace chunk;
    return chunk_inserter_t<Inserter, T>{ di };
}

template <typename T, typename Iter>
inline auto chunk_reader(Iter& it, size_t cnt)
{
    using namespace chunk;
    return chunk_reader_t<Iter, T>{ it, cnt };
}

}

#endif
