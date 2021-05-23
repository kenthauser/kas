#ifndef KAS_ARM_ARM_STMT_H
#define KAS_ARM_ARM_STMT_H

// Declare arm parsed instruction
//
// format is regular:  opcode + [(comma-separated) args]
//
// Opcode `name` can specify condition code and argument width
// Thus, define an `info` to hold this information

#include "arm_arg.h"
#include "arm_hw_defns.h"

#include "target/tgt_insn.h"
#include "target/tgt_stmt.h"

#include "kbfd/target/arm.h"

namespace kas::arm
{
// info: accumulate info from parsing insn not captured in `args`
// NB: bitfields don't zero-init. use `aliagn_as` support type to zero-init
struct arm_stmt_info_t : detail::alignas_t<arm_stmt_info_t, uint16_t>
{
    using base_t::base_t;
    static constexpr auto INFO_CCODE_MASK = 0x1f;

    // get sz() (byte/word/long/etc) for a `mcode`
    uint8_t sz(arm_mcode_t const&) const;

    // test if mcode supported for `info`
    const char *ok(arm_mcode_t const&) const;

    // format `info`
    void print(std::ostream&) const;

    friend std::ostream& operator<<(std::ostream& os, arm_stmt_info_t const& info)
    {
        info.print(os); return os;
    }

    value_t ccode     : 4;      // conditional instruction code
    value_t has_ccode : 1;      // is conditional
    value_t has_sflag : 1;      // opcode has `S` suffix (ie: set flags)
    value_t has_nflag : 1;      // opcode has `.N` suffix (ie: only narrow)
    value_t has_wflag : 1;      // opcode has `.W` suffix (ie: only wide)
    value_t has_bflag : 1;      // unsigned byte
    value_t has_tflag : 1;      // user space access
    value_t has_hflag : 1;      // misc size access
    value_t has_mflag : 1;      // ldm/stm update type
};

// declare result of parsing
using arm_insn_t = tgt::tgt_insn_t<struct arm_mcode_t
                                  , hw::arm_hw_defs
                                  , KAS_STRING("ARM")
                                  >;

struct arm_stmt_t : tgt::tgt_stmt<arm_stmt_t
                                , arm_insn_t
                                , arm_arg_t
                                , arm_stmt_info_t
                                >
{
    using base_t::base_t;
    
    // HOOK: method to validate mcode. Use for `arm_stmt_info_t` validation
    tagged_msg validate_stmt(arm_mcode_t const *mcode_p) const;

    // process info_t from parse
    template <typename Context>
    void operator()(Context const& ctx);

    // suffix info for ldr/str
    uint32_t sfx;
};


}

#endif
