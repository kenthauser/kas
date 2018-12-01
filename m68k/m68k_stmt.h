#ifndef KAS_M68K_STMT_H
#define KAS_M68K_STMT_H

// Boilerplate to allow `statement parser` to accept m68k insns
//
// Each "statment" is placed in `stmt_m68k` structure before being evaluated
//
// Declare m68k parsed instruction type
//
// format is regular:  opcode + [args]

#include "m68k_arg_defn.h"

// XXX clang problem
#define TGT_STMT_NAME stmt_m68k
#define TGT_INSN_T    m68k::m68k_insn_t
#define TGT_ARG_T     m68k::m68k_arg_t

#include "target/tgt_stmt.h"

#include "kas_core/opcode.h"
#include "parser/parser_stmt.h"
#include "parser/annotate_on_success.hpp"

#include <boost/fusion/include/adapt_struct.hpp>
#include <list>

namespace kas::m68k
{
#if 0
    // crashes clang... KBH 2018/11/10
    using stmt_m68k = tgt::tgt_stmt<m68k_insn_t, m68k_arg_t>;
#else

    using tgt::TGT_STMT_NAME;
#endif
}


namespace kas::m68k::parser
{
    namespace x3 = boost::spirit::x3;

    // declare parser for M68K instructions
    using m68k_insn_parser_type = x3::rule<struct _insn, m68k_insn_t const*>;
    BOOST_SPIRIT_DECLARE(m68k_insn_parser_type)

    m68k_insn_parser_type const& m68k_insn_parser();
}

#endif
