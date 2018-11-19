#ifndef KAS_BSD_BSD_INSN_H
#define KAS_BSD_BSD_INSN_H

#include "expr/expr.h"
#include "parser/kas_position.h"
#include "parser/parser_stmt.h"
#include "kas_core/opc_symbol.h"
#include "kas_core/opc_misc.h"

#include "bsd_arg_defn.h"
#include "bsd_ops_core.h"

#include <list>

namespace kas::bsd
{
namespace detail
{
    using bsd_pseudo_tags = meta::list<
          struct bsd_basic_tag
        , struct bsd_macro_tag
        , struct bsd_elf_tag
        , struct bsd_dwarf_tag
        >;

    template <typename tag = void> struct pseudo_ops_v : meta::list<> {};
    template <typename tag = void> struct dwarf_ops_v  : meta::list<> {};

    // forward declare pseudo-op type
    struct pseudo_op_t;
}

using namespace kas::core::opc;

using e_fixed_t = typename expression::e_fixed_t;

struct bsd_stmt_pseudo : kas::parser::insn_stmt<bsd_stmt_pseudo>
{
    struct arg_t {
        detail::pseudo_op_t const *op;
        bsd_args v_args;
    };
    static inline arg_t d;

    static const char *name();

    //template <typename...Ts>
    static opcode& gen_insn(opcode::Inserter& di, opcode::fixed_t& fixed, opcode::op_size_t& size);
    
    static void print_args(::kas::parser::print_fn& fn);
    
    template <typename Context>
    void operator()(Context const& ctx);
};


struct bsd_stmt_label : kas::parser::insn_stmt<bsd_stmt_label, opc_label>
{
    static inline core::symbol_ref ident;

    static auto get_args()
    {
        return std::forward_as_tuple(std::move(ident));
    }
    
    template <typename Context>
    void operator()(Context const& ctx);
};

struct bsd_stmt_equ : kas::parser::insn_stmt<bsd_stmt_equ, opc_equ>
{
    struct arg_t {
        bsd::token_ident ident;
        bsd_arg          value;
    };

    static inline arg_t d;
    
    static auto get_args()
    {
        return std::forward_as_tuple(std::move(d.ident), std::move(d.value));
    }
    
    template <typename Context>
    void operator()(Context const& ctx);
};

struct bsd_stmt_org : kas::parser::insn_stmt<bsd_stmt_org, bsd_org>
{
    static inline bsd_args v_args;

    static auto get_args()
    {
        return std::forward_as_tuple(std::move(v_args));
    }

    template <typename Context>
    void operator()(Context const& ctx);
};

//
// x3 parser hooks: take arg lists & create insn objects
//

template <typename Context>
void bsd_stmt_pseudo::operator()(Context const& ctx)
{
    auto& args = x3::_attr(ctx);
    d.op       = boost::fusion::at_c<0>(args);
    d.v_args   = boost::fusion::at_c<1>(args);
    x3::_val(ctx) = *this;
}


template <typename Context>
void bsd_stmt_label::operator()(Context const& ctx)
{
    ident = x3::_attr(ctx);
    x3::_val(ctx) = *this;
}

template <typename Context>
void bsd_stmt_equ::operator()(Context const& ctx)
{
    auto& args = x3::_attr(ctx);
    d.ident    = std::move(boost::fusion::at_c<0>(args));
    d.value    = std::move(boost::fusion::at_c<1>(args));
    x3::_val(ctx) = *this;
}

template <typename Context>
void bsd_stmt_org::operator()(Context const& ctx)
{
    // .org pseudo-op passed container of args. Emulate.
    v_args = bsd_args();
    v_args.push_back(std::move(x3::_attr(ctx)));
    x3::_val(ctx) = *this;
}

}

#endif
