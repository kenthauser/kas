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

//struct stmt_t : detail::parser_variant, kas_position_tagged
struct stmt_t : kas_position_tagged
{
    using base_t = detail::parser_variant;
#if 0
    using base_t::base_t;
    template <typename...Ts>
    stmt_t(Ts&&...args) : base_t(std::forward<Ts>(args)...) {}
#else
    stmt_t() = default;
    stmt_t(base_t&& var) : var(std::move(var)) {}
    template <typename T,
              typename = std::enable_if_t<meta::in<detail::all_types_l
                                      , std::remove_reference_t<T>>::value>>
    stmt_t(T&& s) : var(std::forward<T>(s)) {}
    //stmt_t(stmt_t const&) = default;
    //template <typename...Ts>
    //stmt_t(Ts&&...args) : var(std::forward<Ts>(args)...) {}

    base_t var;

#endif
    //stmt_t() = default;
    //stmt_t(detail::parser_variant var) : var{std::move(var)} {}

    // instantiate `stmt_t` from error
   // stmt_t(kas_diag_t const& err) : stmt_t({stmt_error(err)}) {}
   //stmt_t(stmt_error err) : var(std::move(err)) {}
    //stmt_t(stmt_error err) : stmt_t(err) {};
    std::string src() const
    {
        return this->where();
    }

    core::core_insn operator()()
    {
        return var.apply_visitor(x3::make_lambda_visitor<core::core_insn>(
            [this](auto&& node) -> core::core_insn
            {
                static core::opc::opc_error error;

                // construct empty insn, location tagged
                core::core_insn insn{*this};     // get loc

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
        var.apply_visitor(x3::make_lambda_visitor<void>(
        //apply_visitor(x3::make_lambda_visitor<void>(
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

private:
    //detail::parser_variant var;
};
}






#endif
