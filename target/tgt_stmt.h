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

template <typename INSN_T, typename ARG_T>
struct tgt_stmt : kas::parser::parser_stmt<tgt_stmt<INSN_T, ARG_T>>
{
    using insn_t   = INSN_T;
    using arg_t    = ARG_T;

    // method used to assemble instruction
    // NB: if arg_t::reset() required, it must be in grammar
    core::opcode *gen_insn(core::opcode::data_t&);

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
