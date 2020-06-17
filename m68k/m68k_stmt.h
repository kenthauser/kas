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
// NB: there are > 32 variants of mac
using m68k_insn_t = tgt::tgt_insn_t<struct m68k_mcode_t, hw::m68k_hw_defs, 64>;

// info: accumulate info from parsing insn not captured in `args`
// NB: bitfields don't zero-init. use `aliagn_as` support type to zero-init
struct m68k_stmt_info_t : detail::alignas_t<m68k_stmt_info_t, uint16_t>
{
    using base_t::base_t;
    using mcode_t = m68k_mcode_t;

    // get sz() (byte/word/long/etc) for a `mcode`
    uint8_t sz(mcode_t const&) const;

    // test if mcode supported for `info`
    const char *ok(mcode_t const&) const;

    void print(std::ostream&) const;

    friend std::ostream& operator<<(std::ostream& os, m68k_stmt_info_t const& info)
    {
        info.print(os); return os;
    }

    // NB: `has_dot` is parse only. 
    value_t arg_size  : 3;      // argument size: 7 == void (not specified)
    value_t ccode     : 5;      // conditional instruction code
    value_t has_ccode : 1;      // is conditional
    value_t fp_ccode  : 1;      // is floating point conditional
};


struct m68k_stmt_t : tgt::tgt_stmt<m68k_stmt_t, m68k_insn_t, m68k_arg_t>
{
    using base_t::base_t;

    // X3 method to initialize instance
    template <typename Context>
    void operator()(Context const& ctx);

    // method validate args. Principally for target & address mode
    template <typename ARGS_T, typename TRACE>
    kas_error_t validate_args(insn_t const&, ARGS_T&, bool& args_arg_const, TRACE * = {}) const;

    // method to validate mcode. Principally for `stmt_info_t` validation
    const char *validate_stmt(mcode_t const *mcode_p) const;

    // utility method to test if floating-point insn
    bool is_fp() const
    {
        return insn_p->name[0] == 'f';
    }
    
    m68k_stmt_info_t flags;
};


}



#endif
