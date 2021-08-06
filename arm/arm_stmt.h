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
struct arm_stmt_info_t : detail::alignas_t<arm_stmt_info_t
                                         , uint16_t
                                         , tgt::tgt_stmt_info_t>
{
    using base_t::base_t;
    
    // special values of condition codes
    static constexpr auto ARM_CC_OMIT = 0xf;    // condition code: not specified
    static constexpr auto ARM_CC_ALL  = 0xe;    // condition code: all specified

    // default info is no-ccode, no suffix
    arm_stmt_info_t() : ccode{ARM_CC_OMIT}, base_t() {}

    // test if mcode supported for `info`
    const char *ok(arm_mcode_t const&) const;

    // XXX should not be needed
    static constexpr uint8_t sz(arm_mcode_t const&) { return {}; }

    // format `info`
    void print(std::ostream&) const;

    friend std::ostream& operator<<(std::ostream& os, arm_stmt_info_t const& info)
    {
        info.print(os); return os;
    }

    value_t ccode     : 4;      // conditional instruction code
    value_t sfx_index : 5;      // index of matched suffix from `arm_sfx_t`
    value_t has_sflag : 1;      // opcode has `S`  suffix (NB: set condition code flags)
    value_t has_nflag : 1;      // opcode has `.N` suffix (NB: only narrow)
    value_t has_wflag : 1;      // opcode has `.W` suffix (NB: only wide)
};

// declare result of parsing
using arm_insn_t = tgt::tgt_insn_t<struct arm_mcode_t
                                  , hw::arm_hw_defs
                                  , KAS_STRING("ARM")
                                  >;

// forward declare "instruction suffix code" type (from "arm_stmt_flags.h")
struct arm_sfx_t;

struct arm_stmt_t : tgt::tgt_stmt<arm_stmt_t
                                , arm_insn_t
                                , arm_arg_t
                                , arm_stmt_info_t
                                >
{
    using base_t::base_t;
    
    // process info_t from parse
    template <typename Context>
    void operator()(Context const& ctx);

    // suffix codes for ldr/str: xlate code in `info` into pointer
    arm_sfx_t const& sfx();

private:
    arm_sfx_t const *_sfx {};
};


}

#endif
