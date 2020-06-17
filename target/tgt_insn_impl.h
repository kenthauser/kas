#ifndef KAS_TARGET_TGT_INSN_IMPL_H
#define KAS_TARGET_TGT_INSN_IMPL_H

#include "tgt_insn.h"
#include "tgt_mcode_defn.h"

namespace kas::tgt
{

// add mcode to insn queue
template <typename OPCODE_T, typename TST_T, unsigned MAX_MCODES, typename INDEX_T>
void tgt_insn_t<OPCODE_T, TST_T, MAX_MCODES, INDEX_T>::
        add_mcode(mcode_t *mcode_p)
{
    auto&& defn_tst = mcode_p->defn().tst;
    if (defn_tst)
        if (auto err = m68k::hw::cpu_defs[defn_tst])
        {
            std::cout << "mcode ignored: " << name << ": err = " << err << std::endl;
            if (!tst)
                tst = defn_tst;
            return;
        }

    mcodes.push_back(mcode_p);

#if 0
    // map mcode -> name
    if (!mcode_p->canonical_insn)
        mcode_p->canonical_insn = index + 1;
#endif
        
    // limit mcodes per insn to bitset size
    if (mcodes.size() > max_mcodes)
        throw std::logic_error("too many machine codes for " + std::string(name));
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
