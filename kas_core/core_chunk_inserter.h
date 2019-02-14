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

    // make sure at least `n` spots remain in current `chunk`
    void reserve(unsigned n)
    {
        // XXX
    }

    // write expression as integral value or expression
    auto& operator= (expr_t&& e)
    {
        if (auto p = e.get_fixed_p())
            *this = *p;
        else
            add_expr(std::move(e));
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

    // return aligned pointer to allocated memory
    template <typename U>
    U* get_wr_ptr()
    {
        auto chunk = &get_wr_chunk();
        bool full  = false;

        auto index = chunk->template get_wr_index<U>(full);
        if (index < 0)
            // type didn't fit, so flush & try again...
            // XXX comma sequence
            return flush(), get_wr_ptr<U>();

        if (full) {
             // if full, re-type chunk as "full"
            *chunk_p = std::move(chunk->full_t());

            // get pointer to new "chunk" location
            chunk = &boost::get<chunk_type::full_type>(*chunk_p);

            // now full, so need new chunk next time
            chunk_p = nullptr;
        }
        last_write_p = &chunk[index];
        return reinterpret_cast<U*>(last_write_p);
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
        auto& chunk  = get_wr_chunk();
        last_write_p = chunk.end();
        if (chunk.add(t)) {
            // here chunk is full.
            // convert boost::variant<type> to a `full` type
            // NB: should just change variant `tag`, not copy data
            *chunk_p = std::move(chunk.full_t());
            chunk_p = nullptr;
        }
    }

    // element incompatible with chunk. flush & add expr_t to underlying queue
    void add_expr (expr_t&& e)
    {
        flush();
        *di++ = std::move(e);
    }

    // return reference to (open) chunk to write data
    auto& get_wr_chunk()
    {
        // need (local) iterator extension which allows `last()`
        // to return reference to last write. True for `forward iterators`
        // and extension to inserter at `opc::opc_back_insert_iterator`
        static_assert(std::is_same<
                          decltype(di.last())
                        , typename std::iterator_traits<Inserter>::reference
                        >::value
                    , "required (local) inserter extension not present");

        if (!chunk_p) {
            // insert empty chunk & remember where
            *di++ = chunk_type{};
            chunk_p = &di.last();
        }

        // access variant data directly
        return boost::get<chunk_type>(*chunk_p);
    }

    void flush()
    {
        last_write_p = nullptr;
        chunk_p      = nullptr;
        n            = 0;
    }
    
    expr_t     *chunk_p {};
    CHUNK_VT   *last_write_p {};
    Inserter&   di;
    short       n {};       // XXX count in fixed area
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

        value_type* get_p()
        {
            if (!is_chunk())
                throw std::runtime_error("chunk_reader_t: get_p() not in block");
            return chunk_p++;
        }
    
        // make sure at least `n` spots remain in current `chunk`
        void reserve(unsigned n)
        {
            // XXX
        }


        template <typename CT = value_type>
        std::enable_if_t<!std::is_void<CT>::value, bool>
        is_chunk ()
        {
            using full_type = typename chunk_type::full_type;

            if (chunk_p == chunk_end) {
                if (empty())
                    return false;

                if (auto p = it->template get_p<chunk_type>()) {
                    chunk_p   = p->begin();
                    chunk_end = p->end();
                    --cnt;
                    ++it;
                } else if (auto full_p = it->template get_p<full_type>()) {
                    chunk_p   = full_p->begin();
                    chunk_end = full_p->end();
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

        CHUNK_VT       *chunk_p {};
        CHUNK_VT const *chunk_end {};

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
