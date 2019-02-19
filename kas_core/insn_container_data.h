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
    using fixed_t   = typename opc::insn_data::fixed_t;
    using op_size_t = typename opc::insn_data::op_size_t;
 
    insn_container_data() = default;
    insn_container_data(core_insn const&);

    // implement inline for now
    uint16_t    opc_index() const    { return _opc_index; }
    uint16_t    cnt()       const    { return _cnt;       }
    op_size_t   size()      const    { return _size;      }
    parser::kas_loc loc()   const    { return _loc;       }

    // fixed can't be compressed. Expose publically
    fixed_t         fixed     {};

private:
    op_size_t       _size     {};
    parser::kas_loc _loc      {};
    uint16_t        _opc_index{};
    uint16_t        _cnt      {};
};


namespace opc
{
#if 0

insn_data::insn_data(insn_container_data)
{
}

#if 0
// don't precalculate `loc` as it's almost never needed
parser::kas_loc loc() const
{
    if (data)
        return data->loc();
    return _opc_loc;
}
#endif

#endif
}


}

#endif

