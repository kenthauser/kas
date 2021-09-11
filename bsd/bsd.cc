// bsd/bsd.cc: instantiate parser elements

#include "bsd_parser_def.h"
#include "bsd_arg_impl.h"
#include "bsd_stmt_impl.h"
#include "bsd_ops_section_impl.h"


namespace kas::bsd::parser::bnf
{
using namespace x3;
using namespace kas::parser;

// expression parsers
BOOST_SPIRIT_INSTANTIATE(dot_parser_x3, iterator_type, expr_context_type)
BOOST_SPIRIT_INSTANTIATE(sym_parser_x3, iterator_type, expr_context_type)

// directive name parsers
BOOST_SPIRIT_INSTANTIATE(comma_op_x3  , iterator_type, stmt_context_type)
BOOST_SPIRIT_INSTANTIATE(space_op_x3  , iterator_type, stmt_context_type)

// stmt parsers
BOOST_SPIRIT_INSTANTIATE(stmt_comma_x3, iterator_type, stmt_context_type)
BOOST_SPIRIT_INSTANTIATE(stmt_space_x3, iterator_type, stmt_context_type)
BOOST_SPIRIT_INSTANTIATE(stmt_equ_x3  , iterator_type, stmt_context_type)
BOOST_SPIRIT_INSTANTIATE(stmt_org_x3  , iterator_type, stmt_context_type)
BOOST_SPIRIT_INSTANTIATE(stmt_label_x3, iterator_type, stmt_context_type)

}

namespace kas::bsd::parser::detail
{
struct xxx
{
    xxx()
    {
        print_type_name{"bsd_dwarf"}.name<_t<comma_ops_v<bsd_dwarf_tag>>>();
    }
} _xxx;
}
