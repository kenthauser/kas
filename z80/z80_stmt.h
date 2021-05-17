#ifndef KAS_Z80_Z80_STMT_H
#define KAS_Z80_Z80_STMT_H

// Declare z80 parsed instruction
//
// format is regular:  opcode + [(comma-separated) args]
//
// Opcode `name` can specify condition code and argument width
// Thus, define an `info` to hold this information

#include "z80_arg.h"
#include "z80_hw_defns.h"

#include "target/tgt_insn.h"
#include "target/tgt_stmt.h"

namespace kas::z80
{
// declare result of parsing
// NB: there are (at least) 17 variants of `ld`
using z80_insn_t = tgt::tgt_insn_t<struct z80_mcode_t
                                  , hw::z80_hw_defs
                                  , KAS_STRING("Z80")
                                  , 32>;

struct z80_stmt_t : tgt::tgt_stmt<z80_stmt_t, z80_insn_t, z80_arg_t>
{
    using base_t::base_t;

};


}

#endif
