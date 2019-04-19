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

#include <iostream>

namespace kas::tgt
{
    // name types used to instantiate the CRTP templates: reg, reg_set, stmt
    using insn_t     = typename ARCH_MCODE::insn_t;
    using arg_t      = typename ARCH_MCODE::arg_t;
    using reg_t      = typename arg_t::reg_t;
    using regset_t   = typename arg_t::regset_t;
    
    // instantiate reg routines referenced from expression parsers
    template const char *tgt_reg<reg_t>::validate(int) const;

    // instantiate reg_set routines referenced from expression parsers
    // NB: error if `regset_t` is `void`
    template      tgt_reg_set<regset_t, reg_t>::tgt_reg_set(reg_t const&, char);
    template auto tgt_reg_set<regset_t, reg_t>::base_t::binop(const char, tgt_reg_set const&) -> derived_t&;
    template auto tgt_reg_set<regset_t, reg_t>::binop(const char, core::core_expr const&)   -> derived_t&;
    template auto tgt_reg_set<regset_t, reg_t>::binop(const char, int)   -> derived_t&;
    
    // instantiate routines referenced from stmt parsers
    template core::opcode *tgt_stmt<insn_t, arg_t>::gen_insn(core::opcode::data_t&);
    template std::string   tgt_stmt<insn_t, arg_t>::name() const;

    // instantiate printers
    template void tgt_reg_set<regset_t, reg_t>::print<std::ostream>(std::ostream&) const;
    template void tgt_reg_set<regset_t, reg_t>::print<std::ostringstream>(std::ostringstream&) const;
}


#endif

