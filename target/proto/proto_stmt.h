#ifndef KAS_PROTO_UC_PROTO_UC_STMT_H
#define KAS_PROTO_UC_PROTO_UC_STMT_H

// Declare PROTO_LC parsed instruction
//
// format is regular:  opcode + [(comma-separated) args]
//
// Opcode `name` can specify condition code and argument width
// Thus, define an `info` to hold this information

#include "PROTO_LC_arg.h"
#include "PROTO_LC_hw_defns.h"

#include "target/tgt_insn.h"
#include "target/tgt_stmt.h"

namespace kas::PROTO_LC
{
#if 0
// EXAMPLE: custom insn `info`
// info: accumulate info from parsing insn not captured in `args`
// NB: bitfields don't zero-init. use `aliagn_as` support type to zero-init
struct PROTO_LC_stmt_info_t : detail::alignas_t<PROTO_LC_stmt_info_t, uint16_t>
{
    using base_t::base_t;

    // get sz() (byte/word/long/etc) for a `mcode`
    uint8_t sz(PROTO_LC_mcode_t const&) const;

    // test if mcode supported for `info`
    const char *ok(PROTO_LC_mcode_t const&) const;

    // format `info`
    void print(std::ostream&) const;

    friend std::ostream& operator<<(std::ostream& os, PROTO_LC_stmt_info_t const& info)
    {
        info.print(os); return os;
    }

    value_t arg_size  : 3;      // argument size: 7 == void (not specified)
    value_t ccode     : 5;      // conditional instruction code
    value_t has_ccode : 1;      // is conditional
    value_t fp_ccode  : 1;      // is floating point conditional
    value_t is_fp     : 1;      // is floating point insn
};
#endif

// declare result of parsing
// NB: there are (at least) 17 variants of `XXX.l`
using PROTO_LC_insn_t = tgt::tgt_insn_t<struct PROTO_LC_mcode_t
                                  , hw::PROTO_LC_hw_defs
                                  , KAS_STRING("PROTO_UC")
                                  , 32>;

struct PROTO_LC_stmt_t : tgt::tgt_stmt<PROTO_LC_stmt_t, PROTO_LC_insn_t, PROTO_LC_arg_t
        // EXAMPLE: , PROTO_LC_stmt_info_t
        >
{
    using base_t::base_t;
    
    // HOOK: method to validate mcode. Use for `PROTO_LC_stmt_info_t` validation
    //tagged_msg validate_stmt(PROTO_LC_mcode_t const *mcode_p) const;

    // HOOK: parser returns "width" as token
    //template <typename Context>
    //void operator()(Context const& ctx);

    // EXAMPLE: override method to test if floating-point insn
    //bool is_fp() const
    //{
    //    return info.is_fp;
    //}
};


}

#endif
