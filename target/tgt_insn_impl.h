#ifndef KAS_TARGET_TGT_INSN_IMPL_H
#define KAS_TARGET_TGT_INSN_IMPL_H

#include "tgt_insn.h"
#include "tgt_mcode_defn.h"

namespace kas::tgt
{

// add mcode to insn list
template <typename O, typename T, typename B, unsigned M, unsigned N, typename I>
void tgt_insn_t<O, T, B, M, N, I>::
        add_mcode(mcode_t *mcode_p)
{
    //std::cout << "tgt_insn::add_mcode: adding " << mcode_p->name();
    //std::cout << ", arch = " << std::dec << +mcode_p->defn_arch() << std::endl;
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
   
    // select proper `arch` based on `mcode_p.defn()`
    auto& p = mcodes_p[mcode_p->defn_arch()];
    if (!p) p = new std::remove_reference_t<decltype(*p)>;
    
    // limit mcodes per insn to bitset size
    if (p->size() >= max_mcodes)
        throw std::logic_error("too many machine codes for " + std::string(name));

    p->push_back(mcode_p);
}

template <typename O, typename T, typename B, unsigned M, unsigned N, typename I>
template <typename OS>
void tgt_insn_t<O, T, B, M, N, I>::
        print(OS& os) const
{
    os << "[" << name << "]";
}

// XXX workaround `NUM_ARCHS` not retrieved from MCODE_T
template <typename O, typename T, typename B, unsigned M, unsigned N, typename I>
void tgt_insn_t<O, T, B, M, N, I>::
        _assert_num_archs() const
{
    static_assert(N == O::NUM_ARCHS
                , "invalid definition for tgt_insn<..., NUM_ARCHS, ...>");
}
}
#endif
