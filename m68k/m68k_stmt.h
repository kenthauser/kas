#ifndef KAS_M68K_M68K_STMT_H
#define KAS_M68K_M68K_STMT_H

// Declare m68k parsed instruction
//
// format is regular:  opcode + [(comma-separated) args]
//
// Opcode `name` can specify condition code and argument width
// Thus, define an `info` to hold this information

#include "m68k_arg.h"
#include "m68k_hw_defns.h"

#include "target/tgt_insn.h"
#include "target/tgt_stmt.h"

namespace kas::m68k
{
using namespace tgt::parser;

// info: accumulate info from parsing insn not captured in `args`
// NB: bitfields don't zero-init. use `aliagn_as` support type to zero-init
struct m68k_stmt_info_t : alignas_t<m68k_stmt_info_t
                                  , uint16_t
                                  , tgt_stmt_info_t>
{
    using base_t::base_t;
    static constexpr auto CC_RAW_BITS = 6;

    m68k_stmt_info_t(value_t cc_raw, struct m68k_mcode_t& mc);

    // get sz() (byte/word/long/etc) for a `mcode`
    uint8_t sz(m68k_mcode_t const&) const;

    // test if mcode supported for `info`
    const char *ok(m68k_mcode_t const&) const;

    value_t get_cc_raw() const
    {
        return has_ccode ? ((1 << 5) | ccode) : 0;
    }

    void set_cc_raw(value_t cc_raw, struct m68k_mcode_t& mc);

    // format `info`
    void print(std::ostream&) const;

    friend std::ostream& operator<<(std::ostream& os, m68k_stmt_info_t const& info)
    {
        info.print(os); return os;
    }

    value_t arg_size  : 3;      // argument size: 7 == void (not specified)
    value_t ccode     : 5;      // conditional instruction code
    value_t has_ccode : 1;      // is conditional
    value_t fp_ccode  : 1;      // is floating point conditional
    value_t is_fp     : 1;      // is floating point insn
    value_t cpid      : 2;      // cpid (for ccode, etc)
};

// declare result of parsing
// NB: there are (at least) 17 variants of `XXX.l`
#if 0
struct m68k_insn_t : tgt::tgt_insn_t<struct m68k_mcode_t
                                  , hw::m68k_hw_defs
                                  , KAS_STRING("M68K")
                                  >
{
    using base_t = tgt_insn_t;
    using base_t::base_t;
//    using base = tgt::tgt_insn_t;
//    using base_t::base_t;
    
    unsigned get_arch() const;
};
#else
using m68k_insn_t = tgt::tgt_insn_t<struct m68k_mcode_t
                                  , hw::m68k_hw_defs
                                  , KAS_STRING("M68K")
                                  >;
#endif

struct m68k_stmt_t : tgt_stmt<m68k_stmt_t
                            , m68k_insn_t
                            , m68k_arg_t
                            , m68k_stmt_info_t
                            >
{
    using base_t::base_t;


    // method to validate mcode. Use for `m68k_stmt_info_t` validation
    const char *validate_stmt(m68k_mcode_t const *mcode_p) const;

    // parser returns "width" as token
    template <typename Context>
    void operator()(Context const& ctx);

    // all and only m68k floating point insns start with `f`
    bool is_fp() const
    {
        return info.is_fp;

        // alternate implementation:
        // switch (parsed_name()[0])
        // {
        //    case 'f': case 'F': return true;
        //    default:
        //        return false;
        // }
    }
   
    // return if `stmt` is CPID insn
    uint8_t get_cpid_arch() const;

    // save "width" suffix (eg ".w") as token (for diagnostics)
    kas::parser::kas_position_tagged width_tok;
};


}

#endif
