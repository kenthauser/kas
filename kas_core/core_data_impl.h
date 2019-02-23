#ifndef KAS_CORE_CORE_DATA_IMPL_H
#define KAS_CORE_CORE_DATA_IMPL_H

#include "core_insn.h"
#include "insn_container_data.h"

namespace kas::core
{

// initalize container from insn
insn_container_data::insn_container_data(core_insn const &insn)
    : _opc_index (insn.opc_index)
    , fixed      (insn.data.fixed)
{
    // XXX not encoding data
    _cnt  = insn.data.insn_expr_data.size() - insn.data.first;
    _size = insn.data.size;
    _loc  = insn.data.loc;
}

// initialize insn from container data
core_insn::core_insn(insn_container_data& container_data)
    : opc_index(container_data.opc_index())
    , data(container_data)
    {}

// initialize insn_data from container data
insn_data::insn_data(insn_container_data& container_data)
    : fixed(container_data.fixed)
    , size(container_data.size())
    , data_p(&container_data)
    , cnt(container_data.cnt())
    , loc(container_data.loc())
    {}

// `insn_data` methods which reference `insn_container_data` methods
// return object, not reference
auto insn_data::iter() const -> Iter
{
    if (data_p)
        return data_p->iter();

    cnt = insn_expr_data.size() - first;
    return begin() + first;
}

auto insn_data::index() const -> std::size_t
{
    return std::distance(begin(), iter());
}

}
   


#endif
