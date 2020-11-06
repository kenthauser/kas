#ifndef KAS_PARSER_H
#define KAS_PARSER_H

// public interface to the statment parser
// typically referenced in `kas_core/assemble.h`

#include "expr/expr.h"
#include "kas_error.h"
#include "parser_stmt.h"
#include "parser_variant.h"
#include <boost/spirit/home/x3.hpp>

namespace kas
{
    namespace x3 = boost::spirit::x3;

    namespace parser
    {
        using stmt_x3 = x3::rule<struct _, stmt_variant> const;
        BOOST_SPIRIT_DECLARE(stmt_x3)

        // declare x3::on_success annotater
        struct annotate_on_success;
        using aos = annotate_on_success;
    }

    // parser public interface
    using parser::stmt_x3;
}

#endif
