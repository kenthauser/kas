#ifndef KAS_ARM_ARM_STMT_H
#define KAS_ARM_ARM_STMT_H

// declare parser "stmt" from generic definintions
// NB: stmt is `insn` + [`args`]

#include "arm_arg.h"
#include "arm_hw_defns.h"

#include "target/tgt_insn.h"
#include "target/tgt_stmt.h"

// Declare insn_t & stmt_t types
namespace kas::arm
{
    // declare result of parsing
    // NB: there are 19 variants of `ld`
    using arm_insn_t = tgt::tgt_insn_t<struct arm_mcode_t, hw::hw_tst, 32>;
    using arm_stmt_t = tgt::tgt_stmt<arm_insn_t, arm_arg_t>;
}


#endif
