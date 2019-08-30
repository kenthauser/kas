#ifndef KAS_CORE_INSN_CONTAINER_DATA_H
#define KAS_CORE_INSN_CONTAINER_DATA_H

#include "insn_data.h"


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
    using fixed_t   = typename insn_data::fixed_t;
    using op_size_t = typename insn_data::op_size_t;
    using Iter      = typename insn_data::Iter;
 
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
    static auto& iter()
    {
        static auto iter = insn_data::begin();
        return iter;
    }

    static auto index()
    {
        return std::distance(insn_data::begin(), iter());
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
    }
    
    static state_t get_state()
    {
        return {iter()};
    }

    static void reinit()
    {
        iter()    = insn_data::begin();
        _loc_base = {};
    }

    // for test fixture multiple-file support
    // clear static variables
    static void clear()
    {
        // clear expression deque & static instances
        insn_data::clear();
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


}

#endif

