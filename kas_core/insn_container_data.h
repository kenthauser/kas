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
    
    // all deque iterators are invalidated on emplace back.
    // query & set data_iter via offset.
    // "initial state" holds index values
    // "state" holds iterators, which are generated after all data inserted
    static auto index(Iter const& it = iter())
    {
        return std::distance(opcode_data::begin(), it);
    }
    
    using initial_state_t = std::tuple<std::size_t>;
    using state_t         = std::tuple<Iter>;

    // methods for walking frags
    static Iter& iter()
    {
        // iterator into "opcode_data" expression list
        static Iter _iter = opcode_data::begin();
        return _iter;
    }

    // get current iterator & counts
    static state_t get_state()
    {
        return { iter() };
    }

    // reset current iterator & counts for new frag
    static void set_state(state_t const& state)
    { 
        iter() = std::get<0>(state);
    }
    
    // methods for setting up `insn_containers`
    static initial_state_t get_initial_state()
    {
        return { opcode_data::end() };
    }

    static void set_initial_state(initial_state_t const& state)
    {
        auto& index = std::get<0>(state);
        iter() = opcode_data::begin() + index;
    }

    // consume instruction
    void advance(core_insn const& insn)
    { 
        //if (insn.loc)
        //    loc_base = insn.data.loc;
        //std::advance(iter(), insn.data.raw_cnt);
        std::advance(iter(), _cnt);
    }

    // change insn size
    void update(op_size_t const& size)
    {
        _size = size;
    }

    void set_error() 
    {
        static const auto idx_error = opc::opc_error().index();
        _opc_index = idx_error;
    }
    
    // implement inline for now
    uint16_t    opc_index() const    { return _opc_index; }
    uint16_t    cnt()       const    { return _cnt;       }
    op_size_t   size()      const    { return _size;      }
    auto&       loc()       const    { return _loc;       }

    // fixed can't be compressed. Expose publically
    fixed_t         fixed     {};

private:
    // manage global running counters
    static inline uint32_t _loc_base;
    
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

    op_size_t       _size     {};
    parser::kas_loc _loc      {};
    uint16_t        _opc_index{};
    uint16_t        _cnt      {};
};

template <typename OS>
OS& operator<<(OS& os, insn_container_data::state_t const& s)
{
    os << "{ expr=" << insn_container_data::index(std::get<0>(s));
    return os << " } ";
}

template <typename OS>
OS& operator<<(OS& os, insn_container_data::initial_state_t const& s)
{
    os << "{ expr=" << std::get<0>(s);
    return os << " } ";
}


}

#endif

