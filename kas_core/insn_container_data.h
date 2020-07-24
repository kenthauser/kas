#ifndef KAS_CORE_INSN_CONTAINER_DATA_H
#define KAS_CORE_INSN_CONTAINER_DATA_H

#include "opcode_data.h"


namespace kas::core
{

// data stored in insn container
// 
// data values may be compressed for redundancy
// access "compressable" values via methods

// XXX store whole types for now.
struct insn_container_data
{
    // name `core_insn` interface type & inherit types
    using fixed_t   = typename opcode_data::fixed_t;
    using op_size_t = typename opcode_data::op_size_t;
    using Iter      = typename opcode_data::Iter;
 
    insn_container_data() = default;
    insn_container_data(core_insn const&);

    // convert `insn` to new type (typically nop)
    void set_opc_index(uint16_t index)
    {
        _opc_index = index;
    }

    // manage global running counters
    static inline uint32_t _loc_base;
    
    using state_t = std::tuple<Iter>;

    // iter is mutable. copy in `insn_data`
    // NB: for empty std::deque, begin() == end()
    
    static Iter& iter(bool get_empty = false)
    {
        static auto empty_iter = opcode_data::begin();
        static auto iter       = opcode_data::end();

        if (get_empty)
            return empty_iter;
        return iter;
    }

    static auto index(Iter const& it = iter())
    {
        if (it == iter(true))
            return 0L;
        return std::distance(opcode_data::begin(), it);
    }

    void advance(core_insn const& insn)
    { 
        //if (insn.loc)
        //    loc_base = insn.data.loc;
        //std::advance(iter(), insn.data.raw_cnt);
        std::advance(iter(), _cnt);
    }

    void update(op_size_t const& size)
    {
        _size = size;
    }

    // set pointers into new frag
    static void set_state(state_t const& state)
    { 
        iter() = std::get<0>(state);

        if (iter() == iter(true))
            iter() = opcode_data::begin();
    }
    
    static state_t get_state(bool at_end = false)
    {
        if (at_end)
            return { opcode_data::end() };
        return {iter()};
    }

    static void reinit()
    {
        iter()    = opcode_data::begin();
        _loc_base = {};
    }

    // for test fixture multiple-file support
    // clear static variables
    static void clear()
    {
        // clear expression deque & static instances
        opcode_data::clear();
        reinit();
    }

    // implement inline for now
    uint16_t    opc_index() const    { return _opc_index; }
    uint16_t    cnt()       const    { return _cnt;       }
    op_size_t   size()      const    { return _size;      }
    auto&       loc()       const    { return _loc;       }

    // fixed can't be compressed. Expose publically
    fixed_t         fixed     {};

private:
    op_size_t       _size     {};
    parser::kas_loc _loc      {};
    uint16_t        _opc_index{};
    uint16_t        _cnt      {};
};

template <typename OS>
OS& operator<<(OS& os, insn_container_data::state_t const& s)
{
    os << "{ iter=" << insn_container_data::index(std::get<0>(s));
    return os << " } ";
    return os;
}


}

#endif

