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
    
    using all_stmts_l  = all_defns<parser_stmt_l>;
    using all_labels_l = all_defns<parser_label_l>;
}

struct stmt_t: kas_position_tagged
{
    stmt_t(parser_stmt *stmt = {}, kas_position_tagged const& loc = {})
            : stmt(stmt), kas_position_tagged(loc) {}

    // allow `diagnostic` to create `stmt_diag` object
    // NB: location tag imported from diagnostic
    stmt_t(kas_diag_t const& diag) : stmt_t(diag.ref()) {}
    stmt_t(kas_error_t const& ref) : kas_position_tagged(ref.get_loc())
    {
        static detail::stmt_diag diag;
        stmt      = &diag;
        diag.diag = ref;
    }

    std::string src() const
    {
        return where();
    }

    // interpret "parsed" statment as kas `core_insn`
    // here "parsed" statement has proper syntax. Check semantics.
    core::core_insn operator()() const
    {
        // init `core_insn` with parsed location
        core::core_insn insn{*this};
        std::cout << "stmt_t::operator(): where = " << src() << std::endl;

        // examine parsed insn for proper semantics
        // `gen_insn` returns pointer to `core::opcode`
        // `insn_data` holds calculated size & opcode argument data
        if (auto op_p = stmt->gen_insn(insn.data))
            insn.opc_index = op_p->index();

        // if size flags error, clear idx
        if (insn.data.size.is_error())
            insn.opc_index = {};

        // if no index (ie semantic error), set size to zero.
        if (!insn.opc_index)
            insn.data.size = {};

        // if `opc_index` is zero, `insn.data.fixed.diag`
        // holds `kas_diag_t` reference with semantic error
        if (!insn.opc_index)
            std::cout << "stmt_t::operator(): error = "
                      << insn.data.fixed.diag
                      << std::endl;


        return insn;
    }
    
    void print(std::ostream& os) const
    {
        print_obj pobj{os};
        os << stmt->name() << "\t";
        stmt->print_args(pobj);
        stmt->print_info(pobj);
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
