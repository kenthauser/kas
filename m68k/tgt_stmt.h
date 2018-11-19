#ifndef KAS_TARGET_TGT_STMT_H
#define KAS_TARGET_TGT_STMT_H

// Boilerplate to allow `statement parser` to accept insns
//
// Each "statment" is placed in `tgt_stmt` structure before being evaluated
//
// format is regular:  opcode + [args]

#include "kas_core/opcode.h"
#include "parser/parser_stmt.h"
#include "parser/annotate_on_success.hpp"

#include <list>

namespace kas::tgt
{

// clang drops core. Use macros for now...
#ifndef TGT_STMT_NAME

#define TGT_STMT_NAME tgt_stmt
#define TGT_STMT_TYPE tgt_stmt<TGT_INSN_T, TGT_ARG_T>

    template <typename TGT_INSN_T, typename TGT_ARG_T>
#else
#define TGT_STMT_TYPE TGT_STMT_NAME
#endif
    struct TGT_STMT_NAME : kas::parser::insn_stmt<TGT_STMT_TYPE>
    {
        using insn_t = TGT_INSN_T;
        using arg_t  = TGT_ARG_T;
        static inline insn_t const *insn_p;
        static inline std::list<arg_t> args;

        // method used to assemble instruction
        static core::opcode& gen_insn(core::opcode::Inserter&
                                    , core::opcode::fixed_t&
                                    , core::opcode::op_size_t&
                                    );

        // methods used by test fixtures
        static const char *name()
        {
            return insn_p->name();
        }

        static auto get_args()
        {
            return std::make_tuple(std::move(args));
        }

        template <typename Context>
        void operator()(Context const& ctx)
        {
            auto& x3_args = x3::_attr(ctx);
            insn_p        = boost::fusion::at_c<0>(x3_args);
            args          = boost::fusion::at_c<1>(x3_args);
            x3::_val(ctx) = *this;
        }
    };
}

#endif
