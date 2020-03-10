#ifndef KAS_CORE_CORE_FIXED_INSERTER_H
#define KAS_CORE_CORE_FIXED_INSERTER_H

#include "opcode.h"
#include "core_chunk_inserter.h"
//#include "core_chunk_types.h"
#include <cstdint>

// Data Inserter for `fixed` data types
//
// These types first store data in the `fixed` area of insns
// before overflowing to `chunk_inserter` to store data in the
// expression queue.
//
// Data stored in the `fixed` area is stored in host byte-order
// as a standard `array`, so that it can be passed to the emit
// backend as a data array. The last element in the array is used
// as a `data count`. 
//
// Since the last array element serves double-duty as a data count,
// it is important that no data value stored in the last array
// element can mimic a valid data count value. If a conflicting data 
// value would occupy that spot, just overflow to `chunk_inserter`.

// NB: As a bonus, since the emit backend can handle a block of data
// as an array of bytes, the `reader` type does not need to be templated
// on `value_t`, which means that the entire `fixed_data` backend does
// not need to be templated on `value_t`. Just pass `fixed` and `chuck`
// data as a block & let the backend sort it out.


namespace kas::core::opc::detail
{
    
using fixed_t      = typename opcode::data_t::fixed_t;

template <typename value_t>
static constexpr auto FIXED_MAX_N = sizeof(fixed_t::fixed)/sizeof(value_t);

//static constexpr auto COUNT_MAGIC = 0x55aa55aa;      // 32-bit value
static constexpr auto COUNT_MAGIC = 0;      // 32-bit value

template <typename value_t>
constexpr auto count_init()
{
    auto bit_mask = FIXED_MAX_N<value_t> - 1;
    return (COUNT_MAGIC & ~bit_mask);
}

// return true if `value` is valid index for byte size `sz`
// make sure all bits outside of "count" range match "MAGIC"
inline bool is_valid_count(uint32_t value, uint16_t sz)
{
    uint32_t byte_mask = (1 << (sz * 8)) - 1;
    unsigned max_n     = sizeof(fixed_t::fixed)/sz;
    uint32_t bit_mask  = max_n - 1;
    
    // XXX for some reason, the overflow doesn't result in -1. ???
    if (byte_mask == 0) byte_mask = ~0;

    uint32_t n = (value ^ COUNT_MAGIC) & byte_mask;
    return !(n & ~bit_mask);
}

// given that value represents index, return count of valid entries
constexpr uint16_t value2count(uint32_t value, uint16_t sz)
{
    unsigned max_n     = sizeof(fixed_t::fixed)/sz;
    uint32_t bit_mask  = max_n - 1;
    return value & bit_mask;
}

// reader method to get count
template <typename T>
constexpr uint16_t get_count(void const *v)
{
    auto p = reinterpret_cast<T const*>(v);
    auto n = FIXED_MAX_N<T>;

    if (n == 0) return 0;

    auto value = p[n-1];
    if (is_valid_count(value, sizeof(T)))
        return value2count(value, sizeof(T));
    return n;
}

constexpr uint16_t get_count(void const *v, uint16_t sz)
{
    switch (sz) {
    case 1:
        return get_count<uint8_t>(v);
    case 2:
        return get_count<uint16_t>(v);
    case 4:
        return get_count<uint32_t>(v);
    case 8:
        return get_count<uint64_t>(v);
    default:
        throw std::logic_error("fixed_inserter_t::get_count(" + std::to_string(sz) + ")");
    };
}

template <typename value_t>
struct fixed_inserter_t
{
    using opc_inserter = typename opcode::data_t::Inserter;

    using chunk_inserter_t = 
            typename core::chunk::chunk_inserter_t<opc_inserter, value_t>;

    // number of entries in the `fixed` part of INSN
    static constexpr auto N = FIXED_MAX_N<value_t>;

    fixed_inserter_t(opc_inserter& di, fixed_t &fixed) : ci(di)
    {
        if (N) {
            // get pointer into `fixed` area
            p = reinterpret_cast<value_t *>(&fixed.fixed);
            p[N-1] = count_init<value_t>();     // count of valid entries
        } else {
            p = nullptr;    // no available entries
        }
    }

    auto& operator= (expression::e_fixed_t value)
    {
        // store initial elements in `fixed` area
        if (p) {
            auto& cnt = p[N-1];

            // save if not last
            auto n = value2count(cnt, sizeof(value_t));
            if (n != (N - 1)) {
                p[n] = value;
                ++cnt;
                return *this;
            }

            // if last, don't increment count
            // if last, don't save if value mimics count
            if (!is_valid_count(value, sizeof(value_t))) {
                cnt = value;
                p = nullptr;
                return *this;
            }
            // fall thru & store in chunk
            p = nullptr;
        }
            
        // use underlying inserter
        *ci++ = value;
        return *this;
    }

    auto& operator= (expr_t const& e)
    {
        p = nullptr;
        *ci++ = e;
        return *this;
    }

    // iterator methods...
    auto& operator*()     { return *this; }
    auto& operator++()    { return *this; }
    auto& operator++(int) { return *this; }

private:
    chunk_inserter_t ci;
    value_t *p;
};


template <typename Iter>
struct fixed_reader_t
{
    // tuple:  pointer, size, count
    using tpl = std::tuple<void const *, uint16_t, uint16_t>;
    
    // XXX `p` initialization suspect
    fixed_reader_t(fixed_t const& fixed, Iter& it, uint16_t cnt, uint16_t value_sz)
        : p(fixed.data), it(it), cnt(cnt), value_sz(value_sz)
        {}

    bool empty() const
    {
        return !p && cnt == 0;
    }

    bool is_chunk() const
    {
        // test if data in fixed area, or if expression type derived from `e_chunk_tag`
        return p || it->apply_visitor(
                x3::make_lambda_visitor<bool>(
                    [](auto& node)
                        { return std::is_base_of<
                              chunk::e_chunk_tag
                            , std::remove_reference_t<decltype(node)>
                            >::value;
                        }));
    }

    auto& get_expr()
    {
        --cnt;
        return *it++;
    }

    tpl get_chunk() 
    {
        if (p) {
            tpl fixed = { p, value_sz, get_count(p, value_sz) };
            p = nullptr;
            return fixed;
        }

        --cnt;

        // dispatch based on actual chunk type (size, full/empty)
        return it++->apply_visitor(
                    x3::make_lambda_visitor<tpl>(
                        [&](auto& node)
                            { return (*this)(node); }
                    ));
    }

private:
    template <typename T>
    tpl operator()(T const& e)
    {
        throw std::runtime_error("fixed_reader: not a chunk");
    }

    template <typename T>
    tpl operator()(chunk::e_chunk_full<T> const& e)
    {
        // completely full chunk: all elements are data
        return { e.begin(), sizeof(T), e.count() };
    }

    template <typename T, typename = void>
    tpl operator()(chunk::e_chunk<T> const& e)
    {
        // partially full chunk: last element holds count
        return { e.begin(), sizeof(T), e.count() };
    }

    Iter&       it;
    void const *p;
    uint16_t    cnt;
    uint16_t    value_sz;
};
}
#endif
