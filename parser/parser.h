#ifndef KAS_PARSER_H
#define KAS_PARSER_H

// public interface to the expression parser

#include "parser_types.h"
#include "parser_stmt.h"
#include <boost/spirit/home/x3.hpp>

namespace kas
{
    namespace x3 = boost::spirit::x3;

    using stmt_t = parser::parser_stmt;
#if 1
    namespace parser
    {
        using parser_type = x3::rule<struct _stmt, stmt_t> const;
        BOOST_SPIRIT_DECLARE(parser_type)
    }

    // parser public interface
    //parser::parser_type const& stmt();
    parser::parser_type stmt();
#endif
}

#endif
