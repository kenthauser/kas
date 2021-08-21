#ifndef KAS_ARM_ARM_DIRECTIVES_H
#define KAS_ARM_ARM_DIRECTIVES_H

// define tokens required to parser ARM directives
//
// ARM `cpu` and `arch` names require expanded character set

#include "parser/token_defn.h"
#include "parser/parser_stmt.h"
#include <meta/meta.hpp>

namespace kas::arm::parser
{
using namespace kas::parser;
#define STR(_)  KAS_STRING(#_)

//
// Declare TOKENs used to parse ARM directives
//

// allow `.` and `-` in ident to match ARM `arch` definitions
using tok_arm_ident  = token_defn_t<STR(ARM_IDENT)>;

//
// Declare directives specific to ARM architecture
//

struct directive_op_t
{
    template <typename NAME, typename FN>
    constexpr directive_op_t(meta::list<NAME, FN>)
        : name { NAME::value }
        , fn   { nullptr }
        {}

    const char *name;
    void *fn;
};

using arm_ops = meta::list<
  meta::list<STR(cpu)         , struct arm_opc_cpu>
, meta::list<STR(eabi_attribute), struct arm_opc_eabi>
, meta::list<STR(arch)        , struct arm_opc_arch>
, meta::list<STR(arm)         , struct arm_opc_arm>
, meta::list<STR(syntax)      , struct arm_opc_syntax>
, meta::list<STR(fpu)         , struct arm_opc_fpu>
#if 0
, meta::list<STR()            , struct arm_opc_>
, meta::list<STR()            , struct arm_opc_>
, meta::list<STR()            , struct arm_opc_>
#endif
>;

//
// Declare ARM architecture "fixed" and "float" directives
//

using arm_fixed = meta::list<meta::list<>
    , meta::list<int32_t, STR(word)>
    , meta::list<int64_t, STR(quad)>
    , meta::list<int64_t, STR(long)>
    , meta::list<int16_t, STR(half)>
    >;

#if 0
using arm_float = meta::list<meta::list<>
    , meta::list<

    >;
#else
using arm_float = meta::id<void>;
#endif

// declare comment and separator for ARM pseudo-op formats
using arm_comment   = meta::list<STR(@)>;
using arm_sep       = meta::list<STR(!)>;
#undef STR
}

namespace kas::parser
{
// Declare specialized `gen_data_p` methods for defined `token_defn_t` types
}

// Add directive definitions to `kas` lists
namespace kas::parser::detail
{
// configure comment and seperator parsers for ARM
template <> struct parser_comment    <void> : arm::parser::arm_comment {};
template <> struct parser_separator  <void> : arm::parser::arm_sep     {};

// configure pseudo-ops for ARM 
template <> struct fixed_directives  <void> : arm::parser::arm_fixed   {};
template <> struct float_directives  <void> : arm::parser::arm_float   {};
}

#endif
