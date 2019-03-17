#ifndef KAS_PARSER_PARSER_VARIANT_H
#define KAS_PARSER_PARSER_VARIANT_H 


#include "parser_stmt.h"
#include "machine_parsers.h"
#include "kas_core/core_insn.h"

#include "kas/defn_utils.h"

#include <boost/spirit/home/x3/support/ast/variant.hpp>
#include <boost/spirit/home/x3/support/utility/lambda_visitor.hpp>

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

struct stmt_t : detail::parser_variant
{
    using base_t = detail::parser_variant;
    using base_t::base_t;
    using base_t::operator=;

    // not sure why base_t::base_t is insufficient
    template <typename...Ts>
    stmt_t(Ts&&...args) : base_t(std::forward<Ts>(args)...) {}

    // instantiate `stmt_t` from error
    stmt_t(kas_error_t err) : stmt_t{stmt_error(err)} {}
    
    std::string src() const
    {
        return apply_visitor(x3::make_lambda_visitor<std::string>(
            [](auto&& node)
            {
                kas_loc const& loc = node;
                if (loc)
                    return loc.where();
                
                std::string result{"UNTAGGED: "};
                result += typeid(node).name();
                return result;
            }
            ));
    }

    core::core_insn operator()()
    {
        return apply_visitor(x3::make_lambda_visitor<core::core_insn>(
            [](auto&& node) -> core::core_insn
            {
                static core::opc::opc_error error;

                core::core_insn insn{node};     // get loc

                // if valid insn, get opc_index
                if (auto op_p = node.gen_insn(insn.data))
                    insn.opc_index = op_p->index();

                // if size flags error, clear idx
                if (insn.data.size.is_error())
                    insn.opc_index = {};

                // if no index, assume `fixed` holds diag
                if (!insn.opc_index)
                {
                    insn.opc_index = error.index();
                    insn.data.size = {};
                }

                return insn;
                }));
    }
    

    void print(std::ostream& os) const
    {
        this->apply_visitor(x3::make_lambda_visitor<void>(
            [&os](auto&& node)
            { 
                print_obj pobj{os};
                os << node.name() << "\t";
                node.print_args(pobj);
            }
        ));
    }
    
    friend std::ostream& operator<<(std::ostream& os, stmt_t const& stmt)
    {
        stmt.print(os);
        return os;
    }
};
}






#endif
