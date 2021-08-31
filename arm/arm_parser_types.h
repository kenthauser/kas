#ifndef KAS_ARM_ARM_PARSER_TYPES_H
#define KAS_ARM_ARM_PARSER_TYPES_H

// Boilerplate to allow `statement parser` to accept tgt insns
//
// Each "statment" is placed in `stmt_t` type before being evaluated

// get target stmt definitions
#include "arm_stmt.h"
#include "arm_directives.h"

// Declare reg_t, insn_t & stmt_t parsers
namespace kas::arm::parser::bnf
{
// parse insn names: defined by `insn_adder`
using arm_insn_x3   = x3::rule<struct _arm_insn, kas_token>;
using arm_dir_op_x3 = x3::rule<struct _arm_op  , tgt_directive_t const *>;
BOOST_SPIRIT_DECLARE(arm_insn_x3, arm_dir_op_x3)

// parse statements: defined in `arm_parser_def.h`
using arm_stmt_x3   = x3::rule<struct _arm_stmt, arm_stmt_t *>;
using arm_dir_x3    = x3::rule<struct _arm_dir , arm_stmt_directive_t *>;
BOOST_SPIRIT_DECLARE(arm_stmt_x3, arm_dir_x3)
}

// Add ARM parsers to `kas::parser` lists
// Conform `kas::parser` to ARM definitions 
namespace kas::parser::detail
{
// statements parsed by parser
template <> struct stmt_ops_l<defn_cpu> : meta::list<
              arm::parser::bnf::arm_stmt_x3
            , arm::parser::bnf::arm_dir_x3
            > {};
}

#endif
