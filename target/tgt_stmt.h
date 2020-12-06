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
    static constexpr uint8_t value() { return 0; }  // no value to store

    // calculate size for insn
    template <typename MCODE_T>
    static constexpr uint8_t sz(MCODE_T const& mc) 
    {
        // default: retrieve size from definition
        return mc.defn().sz();
    }

    template <typename MCODE_T>
    static constexpr const char *ok(MCODE_T const& mc)
    {   
        return {};          // all mcodes match
    }

    void print(std::ostream& os) const
    {
        os << "*None*";
    }
    
    friend std::ostream& operator<<(std::ostream& os, tgt_stmt_info_t const& i)
    { i.print(os); return os; }
};


template <typename DERIVED_T, typename INSN_T, typename ARG_T
        , typename INFO_T = tgt_stmt_info_t>
struct tgt_stmt : kas::parser::parser_stmt
{
    using derived_t = DERIVED_T;
    using base_t    = tgt_stmt;
    using insn_t    = INSN_T;
    using mcode_t   = typename INSN_T::mcode_t;
    using arg_t     = ARG_T;
    using info_t    = INFO_T;

    using insn_tok_t  = typename insn_t::token_t;

    using kas_error_t = parser::kas_error_t;
    using tagged_msg  = parser::tagged_msg;

    // CRTP casts
    auto& derived() const
        { return *static_cast<derived_t const*>(this); }
    auto& derived()
        { return *static_cast<derived_t*>(this); }

    // method used to assemble instruction
    core::opcode *gen_insn(core::opcode::data_t&) override;

    // method validate args. Principally for target & address mode
    template <typename ARGS_T, typename TRACE>
    kas_error_t validate_args(insn_t const&, ARGS_T&, bool& args_arg_const, TRACE * = {});

    // method to validate mcode suitable for stmt.
    // Principally for `hw_tst` & `stmt_info_t` validation
    tagged_msg validate_stmt(mcode_t const *mcode_p) const { return {}; }
   
    // methods used by test fixtures
    std::string name() const override;

    void print_args(parser::print_obj const& p_obj) const override
    {
        p_obj(args);
    }
    
    // allow floating point constants
    bool is_fp() const      { return false; }

    // generate `tgt_stmt` from args (used by parser)
    template <typename Context>
    void operator()(Context const& ctx);
    
    kas::parser::kas_token  insn_tok;
    info_t                  info;
    std::vector<arg_t>      args;
};


}

#endif
