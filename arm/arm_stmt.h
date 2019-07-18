#ifndef KAS_ARM_ARM_STMT_H
#define KAS_ARM_ARM_STMT_H

// declare parser "stmt" from generic definintions
// NB: stmt is `insn` + [`args`]

#include "arm_arg.h"
#include "arm_hw_defns.h"

#include "utility/align_as_t.h"
#include "target/tgt_insn.h"
#include "target/tgt_stmt.h"

// Declare insn_t & stmt_t types
namespace kas::arm
{
// declare ARM instructions
using arm_insn_t = tgt::tgt_insn_t<struct arm_mcode_t, hw::hw_tst, 32>;

// declare result of parsing: allow several ARM suffixes
struct arm_stmt_t : tgt::tgt_stmt<arm_stmt_t, arm_insn_t, arm_arg_t> 
{
    using base_t::base_t;
    
    // X3 method to initialize instance
    template <typename Context>
    void operator()(Context const& ctx);

    // method validate args. Principally for target & address mode
    template <typename ARGS_T, typename TRACE>
    kas_error_t validate_args(insn_t const&, ARGS_T&, bool& args_arg_const, TRACE * = {}) const;

    // method to validate mcode. Principally for target
    template <typename MCODE_T>
    const char *validate_mcode(MCODE_T const *mcode_p) const;
    
    uint32_t get_flags() const { return flags.value(); }
   
    // bitfields don't zero-init. Use support type.
    struct flags_t : detail::alignas_t<flags_t, uint16_t>
    {
        using base_t::base_t;

        value_t ccode     : 4;      // conditional instruction code
        value_t has_ccode : 1;      // is conditional
        value_t has_sflag : 1;      // opcode has `S` suffix (ie: set flags)
        value_t has_nflag : 1;      // opcode has `.N` suffix (ie: only narrow)
        value_t has_wflag : 1;      // opcode has `.W` suffix (ie: only wide)
    } flags;
};

}


#endif
