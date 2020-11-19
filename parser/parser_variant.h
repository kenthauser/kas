#ifndef KAS_PARSER_PARSER_VARIANT_H
#define KAS_PARSER_PARSER_VARIANT_H 


#include "parser_stmt.h"
#include "kas_core/core_expr_type.h"
#include "machine_parsers.h"

#include "kas_core/core_insn.h"

#include "kas/defn_utils.h"

#include <boost/spirit/home/x3/support/ast/variant.hpp>
#include <boost/spirit/home/x3/support/utility/lambda_visitor.hpp>

#include <typeinfo>

namespace kas::parser
{

namespace detail
{
    using namespace meta;
    
    using all_types_l  = all_defns<parser_type_l>;
    using all_stmts_l  = all_defns<parser_stmt_l>;
    using all_labels_l = all_defns<parser_label_l>;

    using parser_variant = apply<quote<x3::variant>, all_types_l>;
}

struct stmt_t: kas_position_tagged
{
    stmt_t(parser_stmt *stmt = {}, kas_position_tagged const& loc = {})
            : stmt(stmt), kas_position_tagged(loc) {}

    std::string src() const
    {
        return where();
    }

    core::core_insn operator()() const
    {
        core::core_insn insn{*this};
        std::cout << "stmt_t::operator(): where = " << src() << std::endl;

        // if valid insn, generate opc_index & size
        if (auto op_p = stmt->gen_insn(insn.data))
            insn.opc_index = op_p->index();

        // if size flags error, clear idx
        if (insn.data.size.is_error())
            insn.opc_index = {};

        // if no index, set size to zero. fixed holds `diag`
        if (!insn.opc_index)
            insn.data.size = {};

        return insn;
    }
    
    void print(std::ostream& os) const
    {
        print_obj pobj{os};
        os << stmt->name() << "\t";
        stmt->print_args(pobj);
    }
    
    friend std::ostream& operator<<(std::ostream& os, stmt_t const& stmt)
    {
        stmt.print(os);
        return os;
    }

private:
    parser_stmt *stmt;
};
}






#endif
