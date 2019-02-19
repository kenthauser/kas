#ifndef KAS_BSD_BSD_STMT_H
#define KAS_BSD_BSD_STMT_H


#include "parser/parser_stmt.h"
#include "bsd_arg_defn.h"

// XXX try to move out of stmt -- don't inline gen_insn
#include "kas_core/opc_symbol.h"
#include "kas_core/opc_misc.h"
#include "bsd_ops_core.h"

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

    // pseudos with "comma"-separated or "space"-separated args
    template <typename tag = void> struct comma_ops_v : meta::list<> {};
    template <typename tag = void> struct space_ops_v : meta::list<> {};

    // forward declare pseudo-op type
    struct pseudo_op_t;
}

using namespace kas::core::opc;

using expression::e_fixed_t;


struct bsd_stmt_pseudo : kas::parser::parser_stmt<bsd_stmt_pseudo>
{
    // pseduos need out-of-line definations
    std::string name() const;
    
    opcode *gen_insn(opcode::data_t& data);
    void print_args(print_obj const& fn) const;
    
    template <typename Context>
    void operator()(Context const& ctx);
    
    detail::pseudo_op_t const *op;
    bsd_args v_args;
};


struct bsd_stmt_label : kas::parser::parser_stmt<bsd_stmt_label>
{
    static inline opc_label opc;

    opcode *gen_insn(opcode::data_t& data)
    {
        opc.proc_args(data, std::move(ident));
        return &opc;
    }
    
    const char *name() const 
    {
        return "BSD_LABEL";
    }
    
    void print_args(print_obj const& fn) const
    {
        fn(ident);
    }

    template <typename Context>
    void operator()(Context const& ctx);
    
    core::symbol_ref ident;
};

struct bsd_stmt_equ : kas::parser::parser_stmt<bsd_stmt_equ>
{
    static inline opc_equ opc;

    opcode *gen_insn(opcode::data_t& data) 
    {
        opc.proc_args(data, ident, value);
        return &opc;
    }
    
    const char *name() const 
    {
        return "BSD_EQU";
    }
    
    void print_args(print_obj const& fn) const
    {
        fn(ident, value);
    }

    template <typename Context>
    void operator()(Context const& ctx);
    
    bsd::token_ident ident;
    bsd_arg          value;
};

struct bsd_stmt_org : kas::parser::parser_stmt<bsd_stmt_org>
{
    static inline bsd_org opc;

    opcode *gen_insn(opcode::data_t& data) 
    {
        opc.proc_args(data, std::move(v_args));
        return &opc;
    }
    
    const char *name() const 
    {
        return "BSD_ORG";
    }
    
    void print_args(print_obj const& fn) const 
    {
        fn(v_args);
    }
    
    template <typename Context>
    void operator()(Context const& ctx);
    
    bsd_args v_args;
};

//
// x3 parser hooks: take arg lists & create insn objects
//

template <typename Context>
void bsd_stmt_pseudo::operator()(Context const& ctx)
{
    auto& args = x3::_attr(ctx);
    op       = boost::fusion::at_c<0>(args);
    v_args   = boost::fusion::at_c<1>(args);
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
    ident      = std::move(boost::fusion::at_c<0>(args));
    value      = std::move(boost::fusion::at_c<1>(args));
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