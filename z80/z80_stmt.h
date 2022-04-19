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
#if 0
// info: accumulate info from parsing insn not captured in `args`
// NB: bitfields don't zero-init. use `aliagn_as` support type to zero-init
struct z80_stmt_info_t : alignas_t<z80_stmt_info_t
                                  , uint8_t
                                  , tgt_stmt_info_t>
{
    using base_t::base_t;

    // no flags needed for Z80 syntax
#if 0
    // get sz() (byte/word/long/etc) for a `mcode`
    uint8_t sz(z80_mcode_t const&) const;

    // test if mcode supported for `info`
    const char *ok(z80_mcode_t const&) const;

    // format `info`
    void print(std::ostream&) const;

    friend std::ostream& operator<<(std::ostream& os, z80_stmt_info_t const& info)
    {
        info.print(os); return os;
    }
#endif
};
#endif
// declare result of parsing
// NB: there are (at least) 17 variants of `ld`
using z80_insn_t = tgt::tgt_insn_t<struct z80_mcode_t
                                  , hw::z80_hw_defs
                                  , KAS_STRING("Z80")
                                  , 32>;

struct z80_stmt_t : tgt_stmt<z80_stmt_t, z80_insn_t, z80_arg_t>
{
    using base_t::base_t;

};


}

#endif
