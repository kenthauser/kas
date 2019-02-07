#ifndef KAS_Z80_Z80_STMT_H
#define KAS_Z80_Z80_STMT_H

// declare parser "stmt" from generic definintions
// NB: stmt is `insn` + [`args`]

#include "z80_arg.h"
#include "z80_hw_defns.h"

#include "target/tgt_insn.h"
#include "target/tgt_stmt.h"

// Declare insn_t & stmt_t types
namespace kas::z80
{
    // declare result of parsing
    // NB: there are 19 variants of `ld`
    using z80_insn_t = tgt::tgt_insn_t<struct z80_mcode, hw::hw_tst, 32>;
    using z80_stmt_t = tgt::tgt_stmt<z80_insn_t, z80_arg_t>;
}


#endif
