#ifndef KAS_PARSER_PARSER_VARIANT_H
#define KAS_PARSER_PARSER_VARIANT_H 


#include "parser_stmt.h"
#include "machine_parsers.h"

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

    template <typename...Ts>
    stmt_t(Ts&&...args) : base_t(std::forward<Ts>(args)...) {}

    // create trampoline to allow `base` methods to work on trampoline
    const char *name() const
    {
        return get_base().name();
    }

    void print_args(print_obj const& p_obj) const
    {
        return get_base().print_args(p_obj);
    }

    core::core_insn gen_insn(opcode::Inserter& di)
    {
        return get_base().gen_insn(di, fixed, op_size);
    }

    kas_position_tagged const& loc() const
    {
        return get_base();
    }
    

    template <typename...Ts>
    opcode& operator()(Ts&&...args)
    {
        return get_base()(std::forward<Ts>(args)...);
    }

private:
    parser_stmt const& get_base() const 
    {
        return apply_visitor(x3::make_lambda_visitor<parser_stmt const&>(
            [](auto&& node) { return node; }
            ));
    }
    
    parser_stmt& get_base()
    {
        return apply_visitor(x3::make_lambda_visitor<parser_stmt&>(
            [](auto&& node) { return node; }
            ));
    }
};

}






#endif
