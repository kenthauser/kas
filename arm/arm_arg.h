#ifndef KAS_ARM_ARM_ARG_DEFN_H
#define KAS_ARM_ARM_ARG_DEFN_H

// Declare arm argument & arg MODES

#include "arm_reg_types.h"
#include "target/tgt_arg.h"

namespace kas::arm
{

// Declare argument "modes"
enum arm_arg_mode : uint8_t
{
// Standard Modes
      MODE_NONE             // 0 when parsed: indicates missing: always zero
    , MODE_ERROR            // 1 set error message
    , MODE_DIRECT           // 2 direct address
    , MODE_INDIRECT         // 3 indirect address
    , MODE_IMMEDIATE        // 4 immediate arg (signed byte/word)
    , MODE_IMMED_QUICK      // 5 immediate arg (stored in opcode)
    , MODE_REG              // 6 register
    , MODE_REG_INDIR        // 7 register indirect
    , MODE_REG_OFFSET       // 8 register + offset (indirect)
    , MODE_REGSET           // 9 register-set 

// Processor required modes
    , MODE_REG_UPDATE       // 10 update register after use (maybe only SP)
    , MODE_SHIFT            // 11 shift instruction

// Required enumeration
    , NUM_ARG_MODES

};

// forward declare for print methods
struct arm_arg_t;

// allow `arm_shift` to be initialized & treaded as `uint16_t`
struct arm_shift : detail::alignas_t<arm_shift, uint16_t>
{
    using base_t::base_t;

    // immed stored in `arg_t`
    template <typename OS>
    void print(OS& os, arm_arg_t const&) const;

    // true if arg is shifted
    operator bool() const
    {
        return value() != 0;
    }

    uint16_t    ext    : 8;
    uint16_t    reg    : 4;
    uint16_t    type   : 2;
    uint16_t    is_reg : 1;
};

// allow `arm_indirect` to be initialized & treaded as `uint16_t`
struct arm_indirect: detail::alignas_t<arm_indirect, uint16_t>
{
    // ARM7 flags (all shifted 20 bits)
    static constexpr auto W_FLAG = 0x02;    // 1 = write-back
    static constexpr auto U_FLAG = 0x08;    // 1 = up (ie add) NB: calcualted
    static constexpr auto P_FLAG = 0x10;    // 1 = pre-index
    static constexpr auto M_FLAG = 0x40;    // 1 = minus (inverse of U_FLAG)
    static constexpr auto R_FLAG = 0x80;    // 1 = use REG

    // shift & immed stored in `arg_t`
    template <typename OS>
    void print(OS& os, arm_arg_t const&) const;
    
    uint16_t    flags : 8;
    uint16_t    reg   : 8;
};


// `REG_T` & `REGSET_T` args also allow `MCODE_T` to lookup types
struct arm_arg_t : tgt::tgt_arg_t<arm_arg_t, arm_arg_mode, arm_reg_t, arm_reg_set>
{
    // inherit default & error ctors
    using base_t::base_t;
    using error_msg = typename base_t::error_msg;

    // declare size of immed args
    static constexpr tgt::tgt_immed_info sz_info [] =
        {
              { 4 }         // 0: WORD // XXX delete completely??
        };

    // override default print
    template <typename OS> void print(OS&) const;
    
    arm_shift    shift;      // support shifts
    arm_indirect indir;      // support indir
};

}

#endif
