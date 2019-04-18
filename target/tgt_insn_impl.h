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



template <typename OPCODE_T, typename TST_T, unsigned MAX_MCODES, typename INDEX_T>
template <typename ARGS_T, typename TRACE_T>
auto  tgt_insn_t<OPCODE_T, TST_T, MAX_MCODES, INDEX_T>::
        validate_args(ARGS_T& args
                    , bool& args_are_const
                    , TRACE_T *trace
                    ) const -> kas_error_t
{
#if 0
    // if no opcodes, then result is HW_TST
    if (mcodes.empty())
        return { tst.name(), args.front() };
#endif

    // if first is dummy, no args to check
    if (args.front().is_missing())
        return {};

    // NB: pickup size from first mcode of insn
    auto sz = mcodes.front()->sz();
    for (auto& arg : args)
    {
        // if not supported, return error
        if (auto diag = arg.ok_for_target(sz))
            return diag;

        // test if constant    
        if (args_are_const)
            if (!arg.is_const())
                args_are_const = false;
    }
    
    return {};
}

}

#endif
