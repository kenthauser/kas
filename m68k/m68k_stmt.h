#ifndef KAS_M68K_M68K_STMT_H
#define KAS_M68K_M68K_STMT_H

// Boilerplate to allow `statement parser` to accept m68k insns
//
// Each "statment" is placed in `stmt_m68k` structure before being evaluated
//
// Declare m68k parsed instruction type
//
// format is regular:  opcode + [args]

#include "m68k_arg.h"
#include "m68k_hw_defns.h"

#include "target/tgt_insn.h"
#include "target/tgt_stmt.h"

namespace kas::m68k
{
// declare result of parsing
// NB: there are  17 variants of `move.l`
//using m68k_insn_t = tgt::tgt_insn_t<struct m68k_mcode_t, hw::hw_tst, 16>;
using m68k_insn_t = tgt::tgt_insn_t<m68k_mcode_t, unsigned, 32>;

struct m68k_stmt_t : tgt::tgt_stmt<m68k_stmt_t, m68k_insn_t, m68k_arg_t>
{
    using base_t::base_t;

    // X3 method to initialize instance
    template <typename Context>
    void operator()(Context const& ctx);

    // method validate args. Principally for target & address mode
    template <typename ARGS_T, typename TRACE>
    kas_error_t validate_args(insn_t const&, ARGS_T&, bool& args_arg_const, TRACE * = {}) const;

    // method to validate mcode. Principally for target
    template <typename MCODE_T>
    const char *validate_mcode(MCODE_T const *mcode_p) const;
    
    // bitfields don't zero-init. Use support type.
    struct flags_t : detail::alignas_t<flags_t, uint16_t>
    {
        using base_t::base_t;

        value_t arg_size  : 3;      // argument size: 7 == void (not specified)
        value_t ccode     : 5;      // conditional instruction code
        value_t has_ccode : 1;      // is conditional
        value_t has_dot   : 1;      // size suffix has `dot` (motorola format)

        // just use `flags_t` as `stmt_info_t`
        auto& info() 
        {
            return *this;
        }
    } flags;

    void print_info(std::ostream& os) const;
};
}



#endif
