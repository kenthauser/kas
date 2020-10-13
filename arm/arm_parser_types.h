#ifndef KAS_ARM_ARM_PARSER_DEFS_H
#define KAS_ARM_ARM_PARSER_DEFS_H

// Boilerplate to allow `statement parser` to accept tgt insns
//
// Each "statment" is placed in `stmt_t` type before being evaluated

// get target stmt definitions
#include "arm_stmt.h"
#include "parser/parser_stmt.h"

// Declare reg_t, insn_t & stmt_t parsers
namespace kas::arm::parser
{
    namespace x3 = boost::spirit::x3;

    // parse insn names: defined by `insn_adder`
    using arm_insn_x3 = x3::rule<struct _, arm_insn_t const *>;
    BOOST_SPIRIT_DECLARE(arm_insn_x3)

    // parse statements: defined in `arm_parser_def.h`
    using arm_stmt_x3 = x3::rule<struct _tag_arm_stmt, arm_stmt_t>;
    BOOST_SPIRIT_DECLARE(arm_stmt_x3)
}


// Add stmt_t parser to `kas::parser` lists
// NB: insn_t parser used to parse statements in `arm_parser_def.h`
namespace kas::parser::detail
{
    // ARM comment and separator characters
    //template<> struct fmt_separator_str<void> : boost::mpl::string<';'> {};
    //template<> struct fmt_comment_str  <void> : boost::mpl::string<'@'> {};
    template<> struct fmt_separator_str<void> : KAS_STRING(";") {};
    template<> struct fmt_comment_str  <void> : KAS_STRING("@") {};


    // target types for stmt variant
    template <> struct parser_type_l<defn_cpu> :
        meta::list<
              arm::arm_stmt_t
            > {};

    // statements parsed by parser
    template <> struct parser_stmt_l<defn_cpu> :
        meta::list<
              arm::parser::arm_stmt_x3
            > {};

    // NB: no arm-specific label parsers
}

#endif
