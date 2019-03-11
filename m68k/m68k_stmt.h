#ifndef KAS_M68K_M68K_STMT_H
#define KAS_M68K_M68K_STMT_H

// Boilerplate to allow `statement parser` to accept m68k insns
//
// Each "statment" is placed in `stmt_m68k` structure before being evaluated
//
// Declare m68k parsed instruction type
//
// format is regular:  opcode + [args]

#include "m68k_arg_defn.h"

#include "target/tgt_insn.h"
#include "target/tgt_stmt.h"

namespace kas::m68k
{
    // declare result of parsing
    // NB: there are  variants of `move.l`
    using m68k_insn_t = tgt::tgt_insn_t<struct m68k_mcode_t, hw::hw_tst, 16>;
    using m68k_stmt_t = tgt::tgt_stmt<m68k_insn_t, m68k_arg_t>;
}

// XXX these defns could use better home
namespace m68k::opc
{
    // declare opcode groups (ie: include files)
    using m68k_insn_defn_groups = meta::list<
          struct OP_M68K_GEN
        , struct OP_M68K_020
        , struct OP_M68K_040
        , struct OP_M68K_060
        , struct OP_M68K_CPU32
        , struct OP_M68K_68881
        , struct OP_M68K_68551
        , struct OP_COLDFIRE
        >;

    template <typename=void> struct m68k_insn_defn_list : meta::list<> {};

    // declare M68K INSN: 6 args, 32 opcodes with same name
    //using m68k_insn_t = tgt::tgt_insn_t<struct m68k_opcode_t, 6, 32>;
}


#endif
