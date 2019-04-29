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
    , MODE_DIRECT           // 2 direct address (ARM: also accepted for immediate arg. sigh)
    , MODE_INDIRECT         // 3 indirect address
    , MODE_IMMEDIATE        // 4 immediate arg (signed byte/word)
    , MODE_IMMED_QUICK      // 5 immediate arg (stored in opcode)
    , MODE_REG              // 6 register
    , MODE_REG_INDIR        // 7 register indirect
    , MODE_REG_OFFSET       // 8 register + offset (indirect)
    , MODE_REGSET           // 9 register-set 

// Required enumeration
    , NUM_ARG_MODES
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
              {  2 }        // 0: WORD
            , {  1 }        // 1: BYTE
        };
};

}

#endif
