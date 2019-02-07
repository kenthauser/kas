#ifndef KAS_PARSER_H
#define KAS_PARSER_H

// public interface to the expression parser

#include "parser_types.h"
#include "parser_stmt.h"
#include <boost/spirit/home/x3.hpp>

namespace kas
{
    namespace x3 = boost::spirit::x3;

    namespace parser
    {
        // forward declare
        struct all_stmts_t;
        using parser_type = x3::rule<struct _stmt, all_stmts_t> const;
        BOOST_SPIRIT_DECLARE(parser_type)
    }

    // parser public interface
    //parser::parser_type const& stmt();
    //parser::parser_type stmt();
}

#endif
