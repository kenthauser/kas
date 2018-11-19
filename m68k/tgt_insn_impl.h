#ifndef KAS_TARGET_TGT_INSN_IMPL_H
#define KAS_TARGET_TGT_INSN_IMPL_H

#include "tgt_insn.h"

namespace kas::tgt
{

template <typename INSN_T, typename ARGS_T>
parser::tagged_msg validate_arg_modes(
                        INSN_T const& insn
                      , parser::kas_position_tagged const& pos
                      , ARGS_T const& args
                      , bool& args_are_const
                      , std::ostream *trace = {}
                      )
{
#if 0
    // if no opcodes, then result is HW_TST
    if (insn.opcodes.empty())
        return { tst.name(), pos };
#endif

    // assume same "size" for all opcodes for same name
    auto sz = insn.opcodes.front()->sz();
    
    // check all args are OK
    for (auto& arg : args)
    {
        if (auto msg = arg.ok_for_target(sz))
            return { msg, arg };
        if (args_are_const)
            if (!arg.is_const())
                args_are_const = false;
    }
    
    return {};
}

// templated definition to cut down on noise in `insn_t` defn
template <typename OPCODE_T, std::size_t _MAX_ARGS, std::size_t MAX_OPCODES, typename TST_T>

template <typename...Ts>
parser::tagged_msg tgt_insn_t<OPCODE_T, _MAX_ARGS, MAX_OPCODES, TST_T>::validate_args(Ts&&...args) const
{
    return validate_arg_modes(*this, std::forward<Ts>(args)...);
}

}
#endif
