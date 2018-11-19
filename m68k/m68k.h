#ifndef KAS_M68K_M68K_H
#define KAS_M68K_M68K_H

// public interface to the m68k parser object
#include "expr/expr.h"
#include "parser/parser.h"

#include "m68k_stmt.h"      // interface to "statement parser"
#include "m68k_options.h"   // command-line options

#include <boost/spirit/home/x3.hpp>

namespace kas
{
    namespace x3 = boost::spirit::x3;

    namespace m68k::parser
    {
        using namespace kas::parser;
        using m68k_stmt_x3 = x3::rule<struct _m68k_stmt, stmt_t>;
        BOOST_SPIRIT_DECLARE(m68k_stmt_x3);
    }

    namespace parser::detail
    {
        // single insn parser. no label parsers
        template <> struct stmt_ops_l<defn_cpu> : meta::list<
            m68k::parser::m68k_stmt_x3
        > {};
    }
}

#endif
