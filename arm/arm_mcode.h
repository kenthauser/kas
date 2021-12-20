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


static constexpr auto SZ_GEN_MASK     = 0x00f0;
static constexpr auto SZ_DEFN_COND    = 0x0010;
static constexpr auto SZ_DEFN_NO_AL   = 0x0020;
static constexpr auto SZ_DEFN_S_FLAG  = 0x0040;
static constexpr auto SZ_DEFN_REQ_S   = SZ_DEFN_S_FLAG;

// flags for suffix instructions
static constexpr auto SZ_DEFN_SFX_SHIFT = 8;
static constexpr auto SZ_DEFN_SFX_MASK  = 0xff << SZ_DEFN_SFX_SHIFT;
static constexpr auto SZ_DEFN_SFX_REQ   = 0x80 << SZ_DEFN_SFX_SHIFT;

static constexpr auto SZ_DEFN_B_FLAG    = 0x01 << SZ_DEFN_SFX_SHIFT;
static constexpr auto SZ_DEFN_T_FLAG    = 0x02 << SZ_DEFN_SFX_SHIFT;
static constexpr auto SZ_DEFN_REQ_T     = 0x83 << SZ_DEFN_SFX_SHIFT;
static constexpr auto SZ_DEFN_REQ_H     = 0x84 << SZ_DEFN_SFX_SHIFT;
static constexpr auto SZ_DEFN_REQ_M     = 0x85 << SZ_DEFN_SFX_SHIFT;
static constexpr auto SZ_DEFN_REQ_I     = 0x86 << SZ_DEFN_SFX_SHIFT;
static constexpr auto SZ_DEFN_L_FLAG    = 0x07 << SZ_DEFN_SFX_SHIFT;
static constexpr auto SZ_DEFN_REQ_B     = 0x88 << SZ_DEFN_SFX_SHIFT;
static constexpr auto SZ_DEFN_REQ_SH    = 0x89 << SZ_DEFN_SFX_SHIFT;
static constexpr auto SZ_DEFN_REQ_SB    = 0x8a << SZ_DEFN_SFX_SHIFT;

// arm insn operand size
enum arm_op_size_t : uint8_t
{
      OP_SIZE_WORD  // default: 32-bit value
    , OP_SIZE_HALF
    , OP_SIZE_BYTE
    , OP_SIZE_QUAD
    , OP_SIZE_SHALF
    , OP_SIZE_SBYTE
    , NUM_OP_SIZE
};

struct arm_defn_info_t
{
    using value_t = uint32_t;
    constexpr arm_defn_info_t(value_t flags, value_t fn_idx)
        : flags{flags}, fn_idx{fn_idx} {}

    value_t flags    : 16;
    value_t fn_idx   : 16;
};

struct arm_mcode_size_t : tgt::tgt_mcode_size_t
{
    static constexpr auto MAX_ARGS  = 6;
    static constexpr auto NUM_ARCHS = 4;    // insn archs
    using mcode_size_t  = uint16_t;     // THUMB emits 16-bit insns
    using defn_info_t   = arm_defn_info_t;
    using val_idx_t     = uint16_t;
    using val_c_idx_t   = uint16_t;
};



struct arm_mcode_t : tgt::tgt_mcode_t<arm_mcode_t
                                    , parser::arm_stmt_t
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

    // provide translation from enum -> name for operation sizes
    static constexpr const char size_names[][4] =
        { "W", "H", "B", "Q", "SH", "SB" };

    // determine `arch` for `defn_t`
    uint8_t defn_arch() const
    {
        return defn().info.flags & SZ_ARCH_MASK;
    }

    template <typename ARGS_T>
    void emit(core::core_emit&, ARGS_T&&, stmt_info_t const&) const;
};

}
#endif

