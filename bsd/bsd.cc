// bsd/bsd.cc: instantiate parser elements

#include "bsd.h"
#include "bsd_parser_def.h"

// #include "m68k/m68k_insn_parser.h"

namespace kas::bsd
{
    namespace parser
    {
        using namespace x3;
        using kas::parser::iterator_type;
        using kas::parser::context_type;
        
        // expression parsers
        BOOST_SPIRIT_INSTANTIATE(dot_parser_x3, iterator_type, context_type)
        BOOST_SPIRIT_INSTANTIATE(sym_parser_x3, iterator_type, context_type)

        // stmt parsers
        BOOST_SPIRIT_INSTANTIATE(pseudo_stmt_x3, iterator_type, context_type)
        BOOST_SPIRIT_INSTANTIATE(dwarf_stmt_x3 , iterator_type, context_type)
        BOOST_SPIRIT_INSTANTIATE(equ_stmt_x3   , iterator_type, context_type)
        BOOST_SPIRIT_INSTANTIATE(org_stmt_x3   , iterator_type, context_type)
        BOOST_SPIRIT_INSTANTIATE(label_stmt_x3 , iterator_type, context_type)
    }

    // instantiate pseudo_op printer for test runners
//    template void stmt_pseudo::print<std::ostream>(std::ostream&);

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
