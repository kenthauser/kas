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

static constexpr auto AZ_GEN_MASK     = 0x00f0;
static constexpr auto SZ_DEFN_COND    = 0x0010;
static constexpr auto SZ_DEFN_NO_AL   = 0x0020;
static constexpr auto SZ_DEFN_S_FLAG  = 0x0040;

// flags for suffix instructions
static constexpr auto SZ_DEFN_MASK    = 0xff << 8;
static constexpr auto SZ_DEFN_B_FLAG  = 0x01 << 8;
static constexpr auto SZ_DEFN_T_FLAG  = 0x02 << 8;
static constexpr auto SZ_DEFN_REQ_T   = 0x03 << 8;
static constexpr auto SZ_DEFN_REQ_H   = 0x04 << 8;
static constexpr auto SZ_DEFN_REQ_M   = 0x05 << 8;

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

struct arm_defn_info_t
{
    using value_t = uint16_t;
    constexpr arm_defn_info_t(value_t flags, value_t info_idx)
        : flags(flags), info_idx(info_idx) {}

    value_t info_idx :  4;
    value_t flags    : 12;
};

struct arm_mcode_size_t : tgt::tgt_mcode_size_t
{
    static constexpr auto MAX_ARGS = 4;
    using mcode_size_t  = uint32_t;
    using defn_info_t   = arm_defn_info_t;
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
    
    // Methods to insert & extract `stmt_info`
    auto code(stmt_info_t stmt_info) const
                -> std::array<mcode_size_t, MAX_MCODE_WORDS>;
    stmt_info_t extract_info(mcode_size_t const *) const;
};

}
#endif

