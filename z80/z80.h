#ifndef KAS_Z80_Z80_H
#define KAS_Z80_Z80_H

// public interface to the z80 parser object
#include "expr/expr.h"
#include "parser/parser.h"

#include "z80_stmt.h"      // interface to "statement parser"
#include "z80_options.h"   // command-line options

#include <boost/spirit/home/x3.hpp>

namespace kas
{
    namespace x3 = boost::spirit::x3;

    namespace z80::parser
    {
        using namespace kas::parser;
        using z80_stmt_x3 = x3::rule<struct _z80_stmt, stmt_t>;
        BOOST_SPIRIT_DECLARE(z80_stmt_x3);
    }

    namespace parser::detail
    {
        // single insn parser. no label parsers
        template <> struct stmt_ops_l<defn_cpu> : meta::list<
            z80::parser::z80_stmt_x3
        > {};
    }
}

#endif
