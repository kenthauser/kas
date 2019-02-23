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

    template <typename...Ts>
    stmt_t(Ts&&...args) : base_t(std::forward<Ts>(args)...) {}
#if 0
    // create trampoline to allow `base` methods to work on trampoline
    const char *name() const
    {
        return "STMT"; //get_base().name();
    }

    void print_args(print_obj const& p_obj) const
    {
        //return get_base().print_args(p_obj);
    }

    kas_position_tagged const& loc() const
    {
        return apply_visitor(x3::make_lambda_visitor<kas_position_tagged const&>(
            [](auto& node) { return node; }
            ));
    }
#endif
    std::string src() const
    {
        return apply_visitor(x3::make_lambda_visitor<std::string>(
            [](auto&& node)
            {
                if (node.handler)
                    return node.where().second;
                
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
                insn.opc_index = node.gen_insn(insn.data)->index();
                if (insn.data.size.is_error())
                {
                    insn.data.size = {};
                    insn.opc_index = {};
                }
                
                if (!insn.opc_index)
                    insn.opc_index = error.index();
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
