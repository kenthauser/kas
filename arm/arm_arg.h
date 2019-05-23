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
#if 0
    , MODE_PRE_INDEXED
    , MODE_PRE_INDEXED_WB
    , MODE_POST_INDEXED_WB
#endif

// Required enumeration
    , NUM_ARG_MODES

};

// support types: help parsing `indirect` and `shift`

// parsed SHIFT formats
enum arm_shift_parsed
{
      ARM_SHIFT_NONE
    , ARM_SHIFT_LSL
    , ARM_SHIFT_LSL_REG
    , ARM_SHIFT_LSR
    , ARM_SHIFT_LSR_REG
    , ARM_SHIFT_ASR
    , ARM_SHIFT_ASR_REG
    , ARM_SHIFT_ROR
    , ARM_SHIFT_ROR_REG
    , ARM_SHIFT_RRX
    , NUM_ARM_SHIFT
};

// allow `arm_shift` to be initialized & treaded as `uint16_t`
struct arm_shift : detail::alignas_t<arm_shift, uint16_t>
{
    using base_t::base_t;

    template <typename OS>
    void print(OS& os) const;

    uint16_t    ext    : 8;
    uint16_t    reg    : 4;
    uint16_t    type   : 2;
    uint16_t    is_reg : 1;
};

enum arm_indir_t : uint8_t
{
      ARM_INDIR_NONE        //  0
    , ARM_INDIR_REG         //  1
    , ARM_INDIR_REG_WB      //  2
    , ARM_PRE_INDEX_IMM     //  3
    , ARM_PRE_INDEX_IMM_WB  //  4
    , ARM_PRE_INDEX_REG     //  5
    , ARM_PRE_INDEX_REG_WB  //  6
    // NB: all post-index are WB. add dummy's to make even values
    , _ARM_POST_INDEX_IMM_X //  7 dummy placeholder
    , ARM_POST_INDEX_IMM    //  8 has writeback
    , _ARM_POST_INDEX_REG_X //  9 dummy
    , ARM_POST_INDEX_REG    // 10 has writeback
    , NUM_ARM_INDIRECT 
};

// support types to facilitate parsing
struct arm_indirect
{
    arm_indirect(arm_indir_t indir = {}) : indir(indir) {}

    // constructor
    template <typename Context>
    void operator()(Context const& ctx);

    template <typename OS>
    void print(OS& os) const;
    
    arm_reg_t   base_reg;
    arm_reg_t   offset_reg;
    expr_t      offset;
    arm_shift   shift;
    arm_indir_t indir;
    bool        sign {};
};

struct arm_shift_arg
{
    arm_shift_arg(int shift_op = {}) : shift_op(shift_op) {}

    // constructor
    template <typename Context>
    void operator()(Context const& ctx);

    uint8_t      shift_op;
    arm_arg_mode mode {MODE_SHIFT};
    expr_t       expr;      // for error
    arm_shift    shift;
};




// `REG_T` & `REGSET_T` args also allow `MCODE_T` to lookup types
struct arm_arg_t : tgt::tgt_arg_t<arm_arg_t, arm_arg_mode, arm_reg_t, arm_reg_set>
{
    // inherit default & error ctors
    using base_t::base_t;
    using error_msg = typename base_t::error_msg;

    // ARM7 flags
    static constexpr auto S_FLAG = 0x01;    // 1 = update status
    static constexpr auto W_FLAG = 0x02;    // 1 = write-back
    static constexpr auto U_FLAG = 0x08;    // 1 = add
    static constexpr auto P_FLAG = 0x10;    // 1 = pre-index

    // default CTOR is std::pair<expr_t, MODE_T>
    // add ARM specific ctors
    arm_arg_t(arm_shift_arg const&);
    arm_arg_t(arm_indirect  const&);

    // declare size of immed args
    static constexpr tgt::tgt_immed_info sz_info [] =
        {
              { 4 }         // 0: WORD // XXX delete completely??
        };

    arm_shift    shift;
    arm_indirect indir;      // indirect types
};

}

#endif
