#ifndef KAS_Z80_Z80_MCODE_H
#define KAS_Z80_Z80_MCODE_H

#include "z80_stmt.h"
#include "target/tgt_mcode.h"

namespace kas::z80
{

// all z80 instructions have a specified "size" of operand
enum m68k_op_size_t
{
      OP_SIZE_WORD
    , OP_SIZE_BYTE
    , NUM_OP_SIZE
};


// override defaults for various sizes
struct z80_mcode_size_t : tgt::tgt_mcode_size_t
{
    using mcode_size_t = uint8_t;
};

// forward declare z80 default mcode arg formatter
namespace opc
{
    struct FMT_X;
}


struct z80_mcode_t : tgt::tgt_mcode_t<z80_mcode_t, z80_stmt_t, error_msg, z80_mcode_size_t>
{
    using BASE_NAME = KAS_STRING("Z80");

    // use default ctors
    using base_t::base_t;

    //
    // override default types
    //

    // XXX prefer `void` as default. Pickup in `opc` subsystem
    using fmt_default = opc::FMT_X;

    //
    // override default methods
    //
#if 1 
    // determine size of immediate arg
    uint8_t sz(stmt_info_t info) const;
#endif
    // prefix is part of `base` machine code size calculation
    // not part of `arg` size calculation
    auto base_size() const
    {
        // NB: sizeof(mcode_size_t) == sizeof(emitted prefix) == 1
        return code_size() + arg_t::prefix != 0;
    }

    // z80: base code & displacement interspersed in output
    template <typename ARGS_T> 
    void emit(core::emit_base&, ARGS_T&&, stmt_info_t const&) const;
};


}
#endif

