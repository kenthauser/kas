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
    using insn_t = INSN_T;
    using arg_t  = ARG_T;

    // method used to assemble instruction
    core::opcode *gen_insn(core::opcode::data_t& data) 
    {
        auto op_p = do_gen_insn(data);
        arg_t::reset();     // clear any static variables
        return op_p;
    }
    
    // actual method to generate opcode;
    core::opcode *do_gen_insn(core::opcode::data_t&);

    // methods used by test fixtures
    const char *name() const 
    {
        return insn_p->name.c_str();
    }

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
    }
    
    insn_t const      *insn_p;
    std::vector<arg_t> args;
};
}

#endif
