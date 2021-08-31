#ifndef KAS_ARM_ARM_DIRECTIVES_H
#define KAS_ARM_ARM_DIRECTIVES_H

// define tokens required to parser ARM directives
//
// ARM `cpu` and `arch` names require expanded character set

#include "parser/token_defn.h"
#include "parser/parser_stmt.h"
#include <meta/meta.hpp>

// forward declare ARM directive "opcode" methods
namespace kas::arm::opc
{
    struct arm_opc_cpu;
    struct arm_opc_eabi;
    struct arm_opc_arch;
    struct arm_opc_arm;
    struct arm_opc_syntax;
    struct arm_opc_fpu;
}

namespace kas::arm::parser
{
using namespace kas::parser;
#define STR(_)  KAS_STRING(#_)

//
// Declare TOKENs used to parse ARM directives
//

// TOKEN which allows `.` and `-` in ident to match ARM `arch` definitions
using tok_arm_ident  = token_defn_t<STR(ARM_IDENT)>;

//
// Declare directives specific to ARM architecture
//

using arm_ops = meta::list<
  meta::list<opc::arm_opc_cpu     , STR(cpu)>
, meta::list<opc::arm_opc_eabi    , STR(eabi_attribute)>
, meta::list<opc::arm_opc_arch    , STR(arch)>
, meta::list<opc::arm_opc_arm     , STR(arm)>
, meta::list<opc::arm_opc_syntax  , STR(syntax)>
, meta::list<opc::arm_opc_fpu     , STR(fpu)>
#if 0
, meta::list<opc::arm_opc_, STR()>
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
