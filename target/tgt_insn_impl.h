#ifndef KAS_TARGET_TGT_INSN_IMPL_H
#define KAS_TARGET_TGT_INSN_IMPL_H

#include "tgt_insn.h"
#include "tgt_mcode_defn.h"

namespace kas::tgt
{

// add mcode to insn list
template <typename OPCODE_T, typename TST_T, unsigned MAX_MCODES, typename INDEX_T>
void tgt_insn_t<OPCODE_T, TST_T, MAX_MCODES, INDEX_T>::
        add_mcode(mcode_t *mcode_p)
{
    // map mcode -> name (NB: mcodes can have several names)
    if (!mcode_p->insn_index)
        mcode_p->insn_index = index + 1;
    
    // validate `mcode` validation test passes
    auto&& defn_tst = mcode_p->defn().tst;
    if (defn_tst)
        if (auto err = (*hw_cpu_p)[defn_tst])
        {
            if (!tst)
                tst = defn_tst;
            return;
        }
    
    // limit mcodes per insn to bitset size
    if (mcodes.size() >= max_mcodes)
        throw std::logic_error("too many machine codes for " + std::string(name));

    mcodes.push_back(mcode_p);
}

template <typename O, typename T, unsigned M, typename I>
template <typename OS>
void tgt_insn_t<O, T, M, I>::
        print(OS& os) const
{
    os << "[" << name << "]";
}

}

#endif
