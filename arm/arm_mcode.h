#ifndef KAS_ARM_ARM_MCODE_H
#define KAS_ARM_ARM_MCODE_H

#include "arm_stmt.h"
#include "target/tgt_mcode.h"

namespace kas::arm
{

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

   uint8_t const sz() const { return _sz & 0; } 
#if 0    
    // arm: base code & displacement interspersed in output
    template <typename ARGS_T> 
    void emit(core::emit_base&
            , mcode_size_t *
            , ARGS_T&&
            , core::core_expr_dot const*
            ) const;
#endif
};


}
#endif

