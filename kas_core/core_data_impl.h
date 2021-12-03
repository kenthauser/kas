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
    _cnt  = insn.data.opcode_expr_data.size() - insn.data.first;
    _size = insn.data.size;
    _loc  = insn.data.loc;
}

// initialize insn from container data
core_insn::core_insn(insn_container_data& container_data)
    : opc_index(container_data.opc_index())
    , data(container_data)
    {}

// initialize insn_data from container data
opcode_data::opcode_data(insn_container_data& container_data)
    : fixed(container_data.fixed)
    , size(container_data.size())
    , data_p(&container_data)
    , cnt(container_data.cnt())
    , loc(container_data.loc())
    {}

// `opcode_data` methods which reference `insn_container_data` methods
// return object, not reference
auto opcode_data::iter() const -> Iter
{
    if (data_p)
        return data_p->iter();

    cnt = opcode_expr_data.size() - first;
    return begin() + first;
}

auto opcode_data::index() const -> std::size_t
{
    return std::distance(begin(), iter());
}

void opcode_data::set_error(const char *msg)
{
    // generate diagnostic & use general routine
    auto& diag = e_diag_t::error(msg, loc);
    set_error(diag);
}

void opcode_data::set_error(e_diag_t const& diag)
{
    // XXX should be able to ostream diag
    std::cout << "opcode_data::set_error: " << diag.message << std::endl;
    
    fixed.diag = diag.ref();    // fixed now correct for error insn
    
    if (data_p)
    {
        // processing from insn container (during relax)
        data_p->set_error();    // convert to error insn
    }
    else
    {
        // processing before container insertion
        // generate an error insn with zero size
        // `loc` set by ctor
        size.set_error();       // `core_insn` does rest
    }
}


}
   


#endif
