#ifndef KAS_ARM_ARM_MCODE_H
#define KAS_ARM_ARM_MCODE_H

#include "arm_stmt.h"
#include "target/tgt_mcode.h"

namespace kas::arm
{

// these are the bits in `mcode_defn`
// see `arm_insn_common.h` for macros to install in insn defn
// declared as 16-bit field (ie `defn_info_t`)
static constexpr auto SZ_ARCH_MASK    = 0x0f;
static constexpr auto SZ_ARCH_ARM     = 0x00;
static constexpr auto SZ_ARCH_THB     = 0x01;
static constexpr auto SZ_ARCH_THB_EE  = 0x02;
static constexpr auto SZ_ARCH_ARM64   = 0x03;

static constexpr auto SZ_DEFN_COND    = 0x0010;
static constexpr auto SZ_DEFN_S_FLAG  = 0x0020;
static constexpr auto SZ_DEFN_NO_AL   = 0x0040;

// flags for LDR/STR instructions
static constexpr auto SZ_DEFN_B_FLAG  = 0x0080;
static constexpr auto SZ_DEFN_T_FLAG  = 0x0100;
static constexpr auto SZ_DEFN_REQ_T   = 0x0200;
static constexpr auto SZ_DEFN_REQ_H   = 0x0400;
static constexpr auto SZ_DEFN_REQ_M   = 0x0800;

// XXX should these be moved to `arm_stmt.h`: part of `info` field
static constexpr auto INFO_CCODE_MASK = 0x0f;
static constexpr auto INFO_S_FLAG     = 0x10;
static constexpr auto INFO_B_FLAG     = 0x20;
static constexpr auto INFO_H_FLAG     = 0x40;
static constexpr auto INFO_M_FLAG     = 0x80;

// single arm insn size
enum arm_op_size_t
{
      OP_SIZE_LONG      // 0
    , OP_SIZE_WORD
    , OP_SIZE_BYTE
    , OP_SIZE_QUAD
    , OP_SIZE_SWORD
    , OP_SIZE_SBYTE
    , NUM_OP_SIZE
};

struct arm_mcode_size_t : tgt::tgt_mcode_size_t
{
    static constexpr auto MAX_ARGS = 4;
    using mcode_size_t  = uint32_t;
    using defn_info_t   = uint16_t;
    using val_idx_t     = uint16_t;
    using val_c_idx_t   = uint16_t;
};

struct arm_mcode_t : tgt::tgt_mcode_t<arm_mcode_t
                                    , arm_stmt_t
                                    , error_msg
                                    , arm_mcode_size_t
                                    >
{
    using BASE_NAME = KAS_STRING("ARM");

    // use default ctors
    using base_t::base_t;

    //
    // override default methods
    //
    
    // Insert condition code & s_flag as required
    auto code(stmt_info_t stmt_info) const
                -> std::array<mcode_size_t, MAX_MCODE_WORDS>;
};

}
#endif

