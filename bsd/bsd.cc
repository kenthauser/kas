// bsd/bsd.cc: instantiate parser elements

#include "bsd_parser_types.h"
#include "bsd_parser_def.h"

namespace kas::bsd
{
    namespace parser
    {
        using namespace x3;
        using kas::parser::iterator_type;
        using kas::parser::stmt_context_type;
        // expression parsers
        BOOST_SPIRIT_INSTANTIATE(dot_parser_x3, iterator_type, expr_context_type)
        BOOST_SPIRIT_INSTANTIATE(sym_parser_x3, iterator_type, expr_context_type)

        // stmt parsers
        BOOST_SPIRIT_INSTANTIATE(stmt_comma_x3, iterator_type, stmt_context_type)
        BOOST_SPIRIT_INSTANTIATE(stmt_space_x3, iterator_type, stmt_context_type)
        BOOST_SPIRIT_INSTANTIATE(stmt_equ_x3  , iterator_type, stmt_context_type)
        BOOST_SPIRIT_INSTANTIATE(stmt_org_x3  , iterator_type, stmt_context_type)
        BOOST_SPIRIT_INSTANTIATE(stmt_label_x3, iterator_type, stmt_context_type)
    }

    // define bsd_arg printer
    std::ostream& operator<<(std::ostream& os, bsd_arg const& arg)
    {
        if (arg.token_idx == 0)
            os << "[EXPR = " << arg.expr;
        else
            os << "[" << bsd_token_names[arg.token_idx - 1];
            
        return os << ": src = \"" << std::string(arg.begin(), arg.end()) << "\"]";
    }
}
