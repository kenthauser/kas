#ifndef KAS_Z80_Z80_STMT_H
#define KAS_Z80_Z80_STMT_H

// Declare z80 parsed instruction
//
// format is regular:  opcode + [(comma-separated) args]
//
// For INTEL syntax:
//      Opcode `name` can specify condition code
// For ZILOG syntax:
//      no `info` passed in opcde. All via args
// Define an `info` to hold this information

#include "z80_arg.h"
#include "z80_hw_defns.h"

#include "target/tgt_insn.h"
#include "target/tgt_stmt.h"

namespace kas::z80
{
using namespace tgt::parser;

// info: accumulate info from parsing insn not captured in `args`
// NB: not needed for Z80 syntax. Will need to define if INTEL sytnax supported

// declare result of parsing
using z80_insn_t = tgt::tgt_insn_t<struct z80_mcode_t
                                  , hw::z80_hw_defs
                                  , KAS_STRING("Z80")
                                  ,  6      // MAX count of ARGS per STMT
                                  , 32      // MAX count of MCODES per INSN
                                  >;

struct z80_stmt_t : tgt_stmt<z80_stmt_t, z80_insn_t, z80_arg_t>
{
    using base_t::base_t;

};


}

#endif
