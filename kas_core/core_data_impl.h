#ifndef KAS_CORE_CORE_DATA_IMPL_H
#define KAS_CORE_CORE_DATA_IMPL_H

#include "core_insn.h"
#include "insn_container_data.h"

namespace kas::core
{

#if 1
// initalize container from insn
insn_container_data::insn_container_data(core_insn const &insn)
    : _opc_index (insn.op.index())
    , fixed      (insn.data.fixed)
{
    // XXX not encoding data
    _cnt  = insn.data.insn_expr_data.size() - insn.data.first;
    _size = insn.data.size;
    _loc  = insn.data._loc;
}

core_insn::core_insn(insn_container_data& container_data)
    : op(opcode::get(container_data.opc_index()))
    , data_p(&container_data)
{

}
#endif
}

#endif
