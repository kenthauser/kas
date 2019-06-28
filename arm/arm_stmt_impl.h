#ifndef KAS_ARM_ARM_STMT_IMPL_H
#define KAS_ARM_ARM_STMT_IMPL_H

#include "arm_stmt.h"
#include "arm_mcode.h"

namespace kas::arm
{

template <typename ARGS_T, typename TRACE_T>
auto  arm_stmt_t::validate_args(insn_t const& insn
                    , ARGS_T& args
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
    auto sz = insn.get_sz();
    
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

// NB: This method rejects single `MCODE` not `STMT`
template <typename MCODE_T>
const char *arm_stmt_t::validate_mcode(MCODE_T const *mcode_p) const
{
    if (auto base_err = base_t::validate_mcode(mcode_p))
        return base_err;

    auto sz = mcode_p->sz();

    std::cout << "validate_mcode: flags = " << std::hex << flags.value() << " sz = " << +sz << std::endl;

    // check condition code, s-flag, and arch match MCODE & mode
    if (flags.has_ccode)
        if (~sz & SZ_DEFN_COND)
            return "condition code not allowed";

    if (flags.has_sflag)
        if (~sz & SZ_DEFN_S_FLAG)
            return "s-flag not allowed";

    if (flags.ccode == 0xe)
        if (sz & SZ_DEFN_NO_AL)
            return "AL condition code not allowed";

    return {};
}
}

#endif
