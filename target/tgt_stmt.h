#ifndef KAS_TARGET_TGT_STMT_H
#define KAS_TARGET_TGT_STMT_H

// Boilerplate to allow `statement parser` to accept machine code insns
//
// format is regular:  opcode_name + [args]

#include "parser/parser_stmt.h"

#include <boost/fusion/include/at.hpp>
#include <vector>

namespace kas::tgt
{

// default a default "info" for arch's which don't derive any info
// from parsed stmt except for `name` and `args`
struct tgt_stmt_info_t
{
    static constexpr uint8_t value() { return 0; }
    static constexpr uint8_t sz()    { return 0; }

    template <typename MCODE_T>
    void bind(MCODE_T const&) const {}

    void print(std::ostream& os) const
    {
        os << "*None*";
    }
    
    friend std::ostream& operator<<(std::ostream& os, tgt_stmt_info_t const& i)
    { i.print(os); return os; }
};


template <typename DERIVED_T, typename INSN_T, typename ARG_T>
struct tgt_stmt : kas::parser::parser_stmt<tgt_stmt<DERIVED_T, INSN_T, ARG_T>>
{
    using derived_t = DERIVED_T;
    using base_t    = tgt_stmt;
    using insn_t    = INSN_T;
    using mcode_t   = typename INSN_T::mcode_t;
    using arg_t     = ARG_T;


    using kas_error_t = parser::kas_error_t;

    // use default `info` as default `flags`
    tgt_stmt_info_t flags;

protected:
    // CRTP casts
    auto& derived() const
        { return *static_cast<derived_t const*>(this); }
    auto& derived()
        { return *static_cast<derived_t*>(this); }

public:
    auto& get_flags()       { return derived().flags; }

    auto& get_info()        { return derived().flags; }
    
    // method used to assemble instruction
    core::opcode *gen_insn(core::opcode::data_t&);

    // method validate args. Principally for target & address mode
    template <typename ARGS_T, typename TRACE>
    kas_error_t validate_args(insn_t const&, ARGS_T&, bool& args_arg_const, TRACE * = {}) const;

    // method to validate mcode. Principally for `stmt_info_t` validation
    // default: not `stmt_info_t`
    constexpr const char *validate_mcode(mcode_t const *mcode_p) const { return {}; }
   
    // statement flags: variable data stored in opcode `name`: eg `ble` (branch if less-than-or-equal)
    // NB: not all architectures use `stmt_flags` to handle cases such as `ble`

    // methods used by test fixtures
    std::string name() const;

    void print_args(parser::print_obj const& p_obj) const
    {
        p_obj(args);
    }

    // X3 method to initialize instance
    template <typename Context>
    void operator()(Context const& ctx)
    {
        auto& x3_args = x3::_attr(ctx);
        insn_p        = boost::fusion::at_c<0>(x3_args);
        args          = boost::fusion::at_c<1>(x3_args);
        x3::_val(ctx) = *this;
    
#if 0
        // XXX Revisit `taging` later
        // XXX `where` from context begins "after" this parse
        // XXX could get `begin` from `insn_p` if it were tagged.
        // NB: where is a boost::iter_range
        auto& where = x3::get<x3::where_context_tag>(ctx);
        auto& error_handler = x3::get<parser::error_handler_tag>(ctx).get();
        error_handler.tag(*this, where.begin(), where.end());
#endif
    }
    
    insn_t const      *insn_p;
    std::vector<arg_t> args;
};


}

#endif
