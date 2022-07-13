#ifndef KAS_Z80_Z80_MCODE_H
#define KAS_Z80_Z80_MCODE_H

#include "z80_stmt.h"
#include "target/tgt_mcode.h"
#include "target/tgt_opc_base.h"

namespace kas::z80
{
// all z80 instructions have a specified "size" of operand
// enumerate them (NB: align with processor definitions if possible)
enum z80_op_size_t
{
      OP_SIZE_WORD      // 0
    , OP_SIZE_BYTE      // 1
    , OP_SIZE_VOID      // -- no size, maps to word
    , NUM_OP_SIZE
};

struct z80_opc_base;    // forward declaration to override `emit`

struct z80_mcode_t : tgt::tgt_mcode_t<z80_mcode_t, z80_stmt_t, error_msg>
{
    using BASE_NAME = KAS_STRING("Z80");
    using opcode_t  = z80_opc_base;

    // use default ctors
    using base_t::base_t;

    //
    // override default methods
    //
    
    // prefix is part of `base` machine code size calculation
    // not part of `arg` size calculation
    auto base_size() const
    {
        // NB: sizeof(mcode_size_t) == sizeof(emitted prefix) == 1
        // 2020/03/14 KBH: w/o cast, method returns `bool` under clang?!? (per lldb)
        return code_size() + int(arg_t::prefix != 0);
    }
};

// Z80: override `tgt_opc_base::do_emit` implementation
struct z80_opc_base : tgt::opc::tgt_opc_base<z80_mcode_t>
{
    // Z80 mixes prefix / base code / offset / etc
    // override default `emit` method
    void do_emit(core::core_emit& base
               , z80_mcode_t const& mcode
               , argv_t& args
               , stmt_info_t info) const override;
};

}
#endif

