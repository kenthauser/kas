#ifndef KAS_M68K_M68K_STMT_H
#define KAS_M68K_M68K_STMT_H

// Boilerplate to allow `statement parser` to accept m68k insns
//
// Each "statment" is placed in `stmt_m68k` structure before being evaluated
//
// Declare m68k parsed instruction type
//
// format is regular:  opcode + [args]

#include "m68k_arg.h"
#include "m68k_hw_defns.h"

#include "target/tgt_insn.h"
#include "target/tgt_stmt.h"

namespace kas::m68k
{
    // declare result of parsing
    // NB: there are  variants of `move.l`
    //using m68k_insn_t = tgt::tgt_insn_t<struct m68k_mcode_t, hw::hw_tst, 16>;
    using m68k_insn_t = tgt::tgt_insn_t<struct m68k_mcode_t, unsigned, 8>;
    using m68k_stmt_t = tgt::tgt_stmt<m68k_insn_t, m68k_arg_t>;
}



#endif
