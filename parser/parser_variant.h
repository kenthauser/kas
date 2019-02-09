#ifndef KAS_PARSER_PARSER_VARIANT_H
#define KAS_PARSER_PARSER_VARIANT_H 


#include "parser_stmt.h"
#include "machine_parsers.h"
//#include "kas_core/core_insn.h"

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

    //template <typename...Ts>
    //stmt_t(Ts&&...args) : base_t(std::forward<Ts>(args)...) {}

    // create trampoline to allow `base` methods to work on trampoline
    const char *name() const
    {
        return "STMT"; //get_base().name();
    }

    void print_args(print_obj const& p_obj) const
    {
        //return get_base().print_args(p_obj);
    }

#if 0
    core::core_insn gen_insn() 
    {
#if 0
        auto& base = get_base();
        core::core_insn insn{base};
#else
        static core::core_insn insn;
#endif
        return insn; // get_base().gen_insn(di, fixed, op_size);
    }
#endif
    kas_position_tagged const& loc() const
    {
        static kas_position_tagged dummy;
        return dummy;// get_base();
    }
    

    template <typename...Ts>
    opcode& operator()(Ts&&...args)
    {
        static opc_nop<> opc;
        return opc;
        //return get_base()(std::forward<Ts>(args)...);
    }
#if 0
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
#endif
};

}






#endif
