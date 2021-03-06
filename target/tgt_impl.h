#ifndef KAS_TARGET_TGT_IMPL_H
#define KAS_TARGET_TGT_IMPL_H

// Instantiate `CRTP` methods
//
// all derived types are looked up using `ARCH_MCODE` which must be defined in `tgt` namespace

#include "target/tgt_reg_impl.h"
#include "target/tgt_regset_impl.h"
#include "target/tgt_stmt_impl.h"
#include "target/tgt_insn_impl.h"
#include "target/tgt_insn_eval.h"
#include "target/tgt_arg_impl.h"
#include "target/tgt_mcode_impl.h"
#include "target/tgt_validate_impl.h"

#include "expr/format_ieee754_impl.h"
#include "expr/literal_types_impl.h"

#include <iostream>

namespace kas::tgt
{
    // name types used to instantiate the CRTP templates: reg, reg_set, stmt
    using stmt_t     = typename ARCH_MCODE::stmt_t;
    using info_t     = typename ARCH_MCODE::stmt_info_t;
    using insn_t     = typename ARCH_MCODE::insn_t;
    using arg_t      = typename ARCH_MCODE::arg_t;
    using reg_t      = typename arg_t::reg_t;
    using regset_t   = typename arg_t::regset_t;
    using regset_ref = typename regset_t::ref_loc_t;
    
    // instantiate reg routines referenced from expression parsers

    // instantiate reg_set routines referenced from expression parsers
    // NB: error if `regset_t` is `void`
    template      tgt_reg_set<regset_t, reg_t, regset_ref>
                    ::tgt_reg_set(reg_t const&, char);
    template auto tgt_reg_set<regset_t, reg_t, regset_ref>
                    ::base_t::binop(const char, derived_t const&) -> derived_t&;
    template auto tgt_reg_set<regset_t, reg_t, regset_ref>
                    ::binop(const char, core::core_expr_t const&) -> derived_t&;
    template auto tgt_reg_set<regset_t, reg_t, regset_ref>
                    ::binop(const char, int)   -> derived_t&;
   
namespace parser
{
    // instantiate routines referenced from stmt parsers
    template core::opcode *tgt_stmt<stmt_t, insn_t, arg_t, info_t>
                                ::gen_insn(core::opcode::data_t&);
    template std::string   tgt_stmt<stmt_t, insn_t, arg_t, info_t>
                                ::name() const;
}
#if 1
    // instantiate printers
    template void tgt_reg_set<regset_t, reg_t, regset_ref>
                    ::print(std::ostream&) const;
//    template void tgt_reg_set<regset_t, reg_t, regset_ref>
//                    ::print<std::ostringstream>(std::ostringstream&) const;
#endif
}


#endif

