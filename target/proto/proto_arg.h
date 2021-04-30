#ifndef KAS_PROTO_UC_PROTO_UC_ARG_DEFN_H
#define KAS_PROTO_UC_PROTO_UC_ARG_DEFN_H

// Declare PROTO_LC argument & arg MODES

#include "PROTO_LC_reg_types.h"
#include "target/tgt_arg.h"

namespace kas::PROTO_LC
{

// Declare argument "modes"
// These must be "defined" as they are referenced in `target/*` code.
// The `enum` values need not be in any preset order.
// The `NUM_ARG_MODES` is the highest value in use. Required enums may
// have values above `NUM_ARG_MODES` if not used
enum PROTO_LC_arg_mode : uint8_t
{
// Standard Modes
      MODE_NONE             // 0 when parsed: indicates missing arg
    , MODE_ERROR            // 1 set error message
    , MODE_DIRECT           // 2 direct address
    , MODE_INDIRECT         // 3 indirect address
    , MODE_IMMEDIATE        // 4 immediate arg (signed byte/word)
    , MODE_IMMED_QUICK      // 5 immediate arg (stored in opcode)
    , MODE_REG              // 6 register
    , MODE_REG_INDIR        // 7 register indirect
    , MODE_REG_OFFSET       // 8 register + offset (indirect)
    , MODE_REGSET           // 9 register-set 
    , MODE_BRANCH           // 10 relative branch size

// Required enumerations
    , NUM_ARG_MODES
    , NUM_BRANCH = 1        // only 1 branch insn
};

// `REG_T` & `REGSET_T` args also allow `MCODE_T` to lookup types
struct PROTO_LC_arg_t : tgt::tgt_arg_t<PROTO_LC_arg_t
                                 , PROTO_LC_arg_mode
                                 , PROTO_LC_reg_t
                                 , PROTO_LC_reg_set_t>
{
    // inherit basic ctors
    using base_t::base_t;

    // EXAMPLE: declare size of immed args: indexed on `PROTO_LC_op_size_t`
    static constexpr tgt::tgt_immed_info sz_info [] =
        {
              {  2 }        // 0: OP_SIZE_WORD
            , {  1 }        // 1: OP_SIZE_BYTE
        };

    // override `target:tgt_arg_t` methods as required.
};

}

#endif
