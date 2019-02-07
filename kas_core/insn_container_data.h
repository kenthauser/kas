#ifndef KAS_CORE_INSN_CONTAINER_DATA_H
#define KAS_CORE_INSN_CONTAINER_DATA_H

#include "insn_data.h"


namespace kas::core
{

// data stored in insn container
// XXX store whole types for now.
// XXX many redundancies to expliot
struct insn_container_data
{
    using fixed_t   = typename opc::insn_data::fixed_t;
    using op_size_t = typename opc::insn_data::op_size_t;
 
    insn_container_data() = default;
    insn_container_data(core_insn);

    uint16_t        opc_index{};
    uint16_t        cnt      {};
    fixed_t         fixed    {};
    op_size_t       _size    {};
    uint32_t        first    {};
    parser::kas_loc loc      {};


    op_size_t size() const;
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

