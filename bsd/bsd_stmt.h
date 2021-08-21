#ifndef KAS_BSD_BSD_STMT_H
#define KAS_BSD_BSD_STMT_H


#include "parser/parser_stmt.h"
#include "bsd_arg_defn.h"

#include "kas_core/opc_symbol.h"
#include "kas_core/opc_misc.h"
#include "bsd_ops_core.h"

namespace kas::bsd
{
namespace detail
{
    using bsd_pseudo_tags = meta::list<
          struct bsd_basic_tag
        , struct bsd_fixed_tag
        , struct bsd_float_tag
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

// NB: all `stmts` must be declared in `bsd_parser_types.h`

struct bsd_stmt_pseudo : kas::parser::parser_stmt
{
    // pseduos need out-of-line name definitions
    std::string name() const override;
    
    opcode *gen_insn(opcode::data_t& data) override;
    void print_args(print_obj const& fn) const override;
    
    template <typename Context>
    void operator()(Context const& ctx);
    
    detail::pseudo_op_t const *op;
    bsd_args v_args;
};


struct bsd_stmt_label : kas::parser::parser_stmt
{
    // labels generate a `opc_label` insn
    opcode *gen_insn(opcode::data_t& data) override
    {
        static opc_label opc;
        opc.proc_args(data, *ident_p, loc);
        return &opc;
    }
    
    std::string name() const override
    {
        return "BSD_LABEL";
    }
    
    void print_args(print_obj const& fn) const override
    {
        fn(*ident_p);
    }

    template <typename Context>
    void operator()(Context const& ctx);
    
    core::core_symbol_t *ident_p;
    parser::kas_loc loc;
};

struct bsd_stmt_equ : kas::parser::parser_stmt
{
    // `equ` statements generate `equ` insns
    opcode *gen_insn(opcode::data_t& data) override
    {
        static opc_equ opc;
        opc.proc_args(data, *ident_p, value);
        return &opc;
    }
    
    std::string name() const override
    {
        return "BSD_EQU";
    }
    
    void print_args(print_obj const& fn) const override
    {
        fn(*ident_p, value);
    }

    template <typename Context>
    void operator()(Context const& ctx);
    
    core::core_symbol_t *ident_p;
    bsd_arg              value;
    parser::kas_loc      loc;
};

struct bsd_stmt_org : kas::parser::parser_stmt
{
    // `org` statements generate `org` insns
    opcode *gen_insn(opcode::data_t& data) override
    {
        static bsd_org opc;
        opc.proc_args(data, std::move(v_args));
        return &opc;
    }
    
    std::string name() const override
    {
        return "BSD_ORG";
    }
    
    void print_args(print_obj const& fn) const override
    {
        fn(v_args);
    }
    
    template <typename Context>
    void operator()(Context const& ctx);
    
    bsd_args v_args;
};

//
// x3 parser hooks: take arg lists & create insn objects
// NB: all templated so single definition restrictions don't apply
//

template <typename Context>
void bsd_stmt_pseudo::operator()(Context const& ctx)
{
    static bsd_stmt_pseudo obj;
    
    auto& args = x3::_attr(ctx);
    obj.op        = boost::fusion::at_c<0>(args);
    obj.v_args    = boost::fusion::at_c<1>(args);
    x3::_val(ctx) = &obj;
}


template <typename Context>
void bsd_stmt_label::operator()(Context const& ctx)
{
    // set object "location" from parsed ident location
    static bsd_stmt_label obj;
    
    auto& ident_tok = x3::_attr(ctx);
    obj.ident_p     = ident_tok.get_p(core::core_symbol_t());
    obj.loc         = ident_tok;
    x3::_val(ctx)   = &obj;
}

template <typename Context>
void bsd_stmt_equ::operator()(Context const& ctx)
{
    static bsd_stmt_equ obj;
    
    auto& args      = x3::_attr(ctx);
    auto& ident_tok = boost::fusion::at_c<0>(args);
    auto& value_tok = boost::fusion::at_c<1>(args);

    obj.ident_p = ident_tok.get_p(core::core_symbol_t());
    obj.loc     = ident_tok;
    obj.value   = value_tok;

    x3::_val(ctx) = &obj;
}

template <typename Context>
void bsd_stmt_org::operator()(Context const& ctx)
{
    static bsd_stmt_org obj;

    // .org pseudo-op passed container of args. Emulate.
    auto& org_tok = x3::_attr(ctx);     // single token: expr
    obj.v_args = bsd_args();
    v_args.push_back(org_tok);          // `org` pseudo-op need single op
    x3::_val(ctx) = &obj;
}

}


#endif
