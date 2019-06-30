#ifndef KAS_ARM_ARM_MCODE_H
#define KAS_ARM_ARM_MCODE_H

#include "arm_stmt.h"
#include "target/tgt_mcode.h"

namespace kas::arm
{

// these are the bits in `DEFN`
static constexpr auto SZ_ARCH_ARM     = 0x00;
static constexpr auto SZ_ARCH_THB     = 0x01;
static constexpr auto SZ_ARCH_THB_EE  = 0x02;
static constexpr auto SZ_ARCH_ARM64   = 0x04;

static constexpr auto SZ_DEFN_COND    = 0x10;
static constexpr auto SZ_DEFN_S_FLAG  = 0x20;
static constexpr auto SZ_DEFN_NW_FLAG = 0x40;
static constexpr auto SZ_DEFN_NO_AL   = 0x80;

// all instructions have a specified "size" of operand
// these are the bits in `SZ`
static constexpr auto SZ_SZ_COND_MASK = 0x0f;
static constexpr auto SZ_SZ_S_FLAG    = 0x10;
static constexpr auto SZ_SZ_N_FLAG    = 0x20;
static constexpr auto SZ_SZ_W_FLAG    = 0x40;

// single arm insn size XXX none?
enum arm_op_size_t
{
      OP_SIZE_WORD
    , NUM_OP_SIZE
};


// override defaults for various sizes
struct arm_mcode_size_t : tgt::tgt_mcode_size_t
{
    static constexpr auto MAX_ARGS = 4;
    using mcode_size_t = uint32_t;
};

// forward declare arm default mcode arg formatter
namespace opc
{
    struct FMT_X;
}


struct arm_mcode_t : tgt::tgt_mcode_t<arm_mcode_t, arm_stmt_t, error_msg, arm_mcode_size_t>
{
    using BASE_NAME = KAS_STRING("ARM");

    // use default ctors
    using base_t::base_t;

    //
    // override default types
    //

    using fmt_default = opc::FMT_X;

    //
    // override default methods
    //
    
    // Insert condition code & s_flag as required
    auto code(uint32_t stmt_flags) const -> std::array<mcode_size_t, MAX_MCODE_WORDS>;
};


}
#endif

