#ifndef KAS_Z80_Z80_STMT_H
#define KAS_Z80_Z80_STMT_H

// Boilerplate to allow `statement parser` to accept z80 insns
//
// Each "statment" is placed in `stmt_z80` structure before being evaluated
//
// Declare z80 parsed instruction type
//
// format is regular:  opcode + [args]

#include "z80_arg_defn.h"

// XXX clang problem
#define TGT_STMT_NAME stmt_z80
#define TGT_INSN_T    z80::z80_insn_t
#define TGT_ARG_T     z80::z80_arg_t

#include "target/tgt_stmt.h"

#include "kas_core/opcode.h"
#include "parser/parser_stmt.h"
#include "parser/annotate_on_success.hpp"

namespace kas::z80
{
#if 0
    // crashes clang... KBH 2018/11/10
    using stmt_z80 = tgt::tgt_stmt<z80_insn_t, z80_arg_t>;
#else

    using tgt::TGT_STMT_NAME;
#endif
}


namespace kas::z80::parser
{
    namespace x3 = boost::spirit::x3;

    // declare parser for M68K instructions
    using z80_insn_parser_type = x3::rule<struct _insn, z80_insn_t const*>;
    BOOST_SPIRIT_DECLARE(z80_insn_parser_type)

    z80_insn_parser_type const& z80_insn_parser();
}

#endif
