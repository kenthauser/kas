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
struct tgt_stmt : kas::parser::insn_stmt<tgt_stmt<INSN_T, ARG_T>>
{
    using insn_t = INSN_T;
    using arg_t  = ARG_T;

//#define static
//#define inline
    static inline insn_t const      *insn_p;
    static inline std::vector<arg_t> args;

    // method used to assemble instruction
    static core::opcode& gen_insn(core::opcode::Inserter&   di
                                , core::opcode::fixed_t&    fixed
                                , core::opcode::op_size_t&  size
                                )
    {
        core::opcode& op = insn_p->gen_insn(di, fixed, size, std::move(args));
        arg_t::reset();     // clear any static variables
        return op;
    }

    // methods used by test fixtures
    static const char *name()
    {
        return insn_p->name.c_str();
    }

    static auto get_args()
    {
        return std::make_tuple(std::move(args));
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
#undef static
#undef inline
};
}

#endif
