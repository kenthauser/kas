#ifndef KAS_BSD_BSD_STMT_IMPL_H
#define KAS_BSD_BSD_STMT_IMPL_H

#include "parser/parser.h"
#include "bsd_elf_defns.h"
#include "bsd_symbol.h"
#include "pseudo_ops_def.h"

#include "parser/kas_token.h"
#include "parser/token_parser.h"
#include "parser/annotate_on_success.hpp"

#include <boost/spirit/home/x3.hpp>

//#include "pseudo_ops_def.h"

namespace kas::bsd::parser
{
namespace detail
{
    // support routines for `pseudo_op_t`
    static constexpr short to_short(const char *) { return 0; }
    template <short N>
    static constexpr short to_short(std::integral_constant<short, N>) { return N; }
                                    
    template <typename OP>
    static auto& get_opcode()
    {
        static OP opcode;
        return opcode;
    }
    
    template <typename OP>
    static opcode& get_ref()
    {
        return get_opcode<OP>();
    }

    // type for pseudo-op definitions
    struct pseudo_op_t
    {
        static constexpr auto MAX_FIXED_ARGS = 2;

        template <typename NAME, typename OPCODE, typename...ARGS>
        constexpr pseudo_op_t(meta::list<NAME, OPCODE, ARGS...>)
            : name      { NAME::value         }
            , arg_c     { sizeof...(ARGS)     }
            , str_v     { ARGS()...           }
            , num_v     { to_short(ARGS())... }
            , op        { get_ref<OPCODE>     }
            , proc_args { &pseudo_op_t::do_proc<OPCODE> } 
            {}

        template <typename OP, typename...Ts>
        opcode& do_proc(opcode::data_t& data, bsd_args&& args) const
        {
            auto& op = get_opcode<OP>();
            op.proc_args(data, std::move(args), arg_c, str_v, num_v);
            return op;
        }

        const char *name;
        opcode&   (*op)();
        opcode&   (pseudo_op_t::*proc_args)(opcode::data_t&, bsd_args&&) const;
        const char *str_v[MAX_FIXED_ARGS];
        short       num_v[MAX_FIXED_ARGS];
        short       arg_c;
    };

    using comma_defs = all_defns<comma_ops_v, bsd_pseudo_tags>;
    using space_defs = all_defns<space_ops_v, bsd_pseudo_tags>;

    static const auto comma_ops = parser::sym_parser_t<pseudo_op_t, comma_defs>();
    static const auto space_ops = parser::sym_parser_t<pseudo_op_t, space_defs>();
}

}

namespace kas::bsd::parser//::bnf
{
auto const comma_op_def = no_case[lexeme['.' >> detail::comma_ops.x3()]];
auto const space_op_def = no_case[lexeme['.' >> detail::space_ops.x3()]];
BOOST_SPIRIT_DEFINE(comma_op, space_op)
}
//
// implement bsd_stmt methods for pseudo-ops
//
namespace kas::bsd
{
std::string bsd_stmt_pseudo::name() const
{
    return std::string("BSD_PSEUDO:") + op->name;
}

opcode *bsd_stmt_pseudo::gen_insn(opcode::data_t& data)
{
    return &(op->*op->proc_args)(data, std::move(v_args));
}

void bsd_stmt_pseudo::print_args(kas::parser::print_obj const& fn) const
{
    // if fixed args, copy to container
    auto n = op->arg_c;
    std::vector<const char *> xtra_args(op->str_v, op->str_v + n);
    
    if (n)
        fn(xtra_args, v_args);
    else
        fn(v_args);
}
    
}

#endif
