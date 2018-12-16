#ifndef KAS_Z80_Z80_PARSER_DEF_H
#define KAS_Z80_Z80_PARSER_DEF_H

// Complex instruction set processors are called "complex" for a reason.
//
// The MIT z80 indirect format allows great flexibility in specifying
// the operands. This code normalizes the various ways of specifying
// arguments into the `z80_parsed_arg_t` type. The code in module
// `z80::z80_parser_support.h` analyzes the various components, processor
// addressing modes, and run-time configuration to generate a
// `z80_arg_t` which is used by all other code.
//
// The MIT direct format uses '#', '@', '@+', '@-' prefixes/postfixes
// to inform interpretation of the single argument. The class
// `z80_parsed_arg_t` processes the single argument (plus pre/postfix)
// arguments.
//
// The indirect formats are generally used to specify register indirect
// displacement and register indirect index operations. These are quite
// complex in their general form.
//
// 1. The indirect format allows an optionally suppressed base register
// (normally an address register or PC, but also on an 020+, a data register)
// to be named.
//
// 2. In addition to a base register, one or two additional "indirection"
// operations may be specified.
//
// 3. Each of the "indirection" operations may specify an optionally
// sized (word/long) and scaled (x1,2,4,8) "general register" (aka the
// index register) and a displacement. The displacement and register
// may be in either order, but each may only be specified once.
//
// 4. The "index" register may be named in either the first or second
// indirect specification, but not both. If a data register is used for
// the base register, it is implicitly added to the first index specification
// and the base register is suppressed.
//
// 5. Long multiply & divides can involve 64-bit operands/results which
// require two data registers: two registers separated by a `:`
//
// 6. Bit Field operators require an bit-offset and bitfield-width
// Offset & width can be constants (0-31) or data registers.
//
// Other complexities apply. Implementation issues include offset values
// which become zero (or go out of range) during evaluation. Not all formats
// are supported by all processors in the family.
//
// In other words...complex.

#ifdef BOOST_SPIRIT_X3_DEBUG
#include <iostream>
#endif

#include "z80.h"

#include "expr/expr.h"              // expression public interface
#include "parser/annotate_on_success.hpp"

#include <boost/spirit/home/x3.hpp>
#include <boost/fusion/include/adapt_struct.hpp>


BOOST_FUSION_ADAPT_STRUCT(
    kas::z80::z80_arg_t,
    expr,
    mode
)


namespace kas::z80::parser
{
    namespace x3 = boost::spirit::x3;
    using namespace x3;
    using namespace kas::parser;

    //////////////////////////////////////////////////////////////////////////
    // Parse Z80 argument using Zilog syntax
    //////////////////////////////////////////////////////////////////////////


    //
    // X3 parser rules: convert "parsed" args to location-stampped args
    //
    
    x3::rule<class z80_arg,     z80_arg_t>  z80_arg     = "z80_arg";
    x3::rule<class z80_missing, z80_arg_t>  z80_missing = "z80_missing";

    auto const z80_arg_def =
              '(' > expr() > ')' > attr(MODE_INDIRECT) 
            | '#' > expr() >       attr(MODE_DIRECT)
            | expr()       >       attr(MODE_DIRECT)
            ;

    auto const z80_missing_def = eps;      // need location tagging


    // tag location for each argument
    struct z80_arg     : kas::parser::annotate_on_success {};
    struct z80_missing : kas::parser::annotate_on_success {};


    BOOST_SPIRIT_DEFINE(z80_arg, z80_missing)

    // an z80 instruction is "opcode" followed by comma-separated "arg_list"
    // no arguments indicated by location tagged `z80_arg` with type MODE_NONE
    
    rule<class _z80_args, std::list<z80_arg_t>> const z80_args = "z80_args";
    auto const z80_args_def
           //= (z80_arg >> *((',' > z80_arg) 
           = z80_arg % ','
           | repeat(1)[z80_missing]        // MODE_NONE, but location tagged
           ;

    BOOST_SPIRIT_DEFINE(z80_args)

    auto const z80_stmt_def = (z80_insn_parser() > z80_args)[stmt_z80()];

    // Parser interface
    z80_stmt_x3 z80_stmt {"z80_stmt"};
    
    BOOST_SPIRIT_DEFINE(z80_stmt)
}

#endif
