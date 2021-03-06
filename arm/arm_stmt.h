#ifndef KAS_ARM_ARM_STMT_H
#define KAS_ARM_ARM_STMT_H

// Declare ARM parsed statement 
//
// format is regular:  opcode + [(comma-separated) args]
//
// Opcode `name` can specify condition code and "suffixes"
// Define an `info` to hold this information

// "suffixes" are primarily condition-codes and "pre-UAL" codes
// which must be parsed separate from BASE code. 

#include "arm_arg.h"
#include "arm_hw_defns.h"

#include "target/tgt_insn.h"
#include "target/tgt_stmt.h"

#include "kbfd/target/arm.h"

namespace kas::arm::parser
{

using namespace tgt::parser;

// info: accumulate info from parsing insn not captured in `args`
// NB: bitfields don't zero-init. use `aliagn_as` support type to zero-init
struct arm_stmt_info_t : alignas_t<arm_stmt_info_t
                                , uint16_t          // exposed as `value_t`
                                , tgt_stmt_info_t>
{
    using base_t::base_t;
    
    // special values of condition codes
    static constexpr auto ARM_CC_OMIT = 0xf;    // condition code: not specified
    static constexpr auto ARM_CC_ALL  = 0xe;    // condition code: all specified

    // default info is no-ccode, no suffix
    arm_stmt_info_t() : ccode{ARM_CC_OMIT}, base_t() {}

    // test if mcode supported for `info`
    // XXX wrong place??
    const char *ok(arm_mcode_t const&) const;

    // XXX should not be needed
    static constexpr uint8_t sz(arm_mcode_t const&) { return {}; }

    // format `info`
    void print(std::ostream&) const;

    friend std::ostream& operator<<(std::ostream& os, arm_stmt_info_t const& info)
    {
        info.print(os); return os;
    }

    value_t sfx_index : 8;      // index of matched suffix from `arm_sfx_t`
    value_t ccode     : 4;      // conditional instruction code
    value_t has_sflag : 1;      // opcode has `S`  suffix (NB: set condition flags)
    value_t has_nflag : 1;      // opcode has `.N` suffix (NB: only narrow)
    value_t has_wflag : 1;      // opcode has `.W` suffix (NB: only wide)
};

// declare result of parsing
// XXX should figure a way to pick up `MCODE` constants instead
// XXX of redeclaring. Requires more than forward declaration of `mcode_t`
// XXX posible solution: move all "references" to *impl.h files
using arm_insn_t = tgt::tgt_insn_t<struct arm_mcode_t
                                  , hw::arm_hw_defs
                                  , KAS_STRING("ARM")
                                  , 6           // MAX_ARGS
                                  , 32          // mcodes per insn per arch
                                  , 5           // insn archs    
                                  >;

// forward declare "instruction suffix code" type (from "arm_stmt_flags.h")
struct arm_sfx_t;

struct arm_stmt_t : tgt_stmt<arm_stmt_t
                           , arm_insn_t
                           , arm_arg_t
                           , arm_stmt_info_t
                           >
{
    using base_t::base_t;
    
    // process parse of statement
    template <typename Context>
    void operator()(Context const& ctx);

    // suffix codes for ldr/str: xlate code in `info` into pointer
    arm_sfx_t const& sfx();

private:
    arm_sfx_t const *_sfx {};
};

// for `ARM` directives
using arg_t = kas_token;
using arg_v = std::vector<arg_t>;

struct arm_stmt_directive_t : tgt_stmt_directive<arm_stmt_directive_t>
{
    using base_t::base_t;
};


}

#endif
