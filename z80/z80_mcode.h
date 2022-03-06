#ifndef KAS_Z80_Z80_MCODE_H
#define KAS_Z80_Z80_MCODE_H

#include "z80_stmt.h"
#include "target/tgt_mcode.h"

namespace kas::z80
{
// all z80 instructions have a specified "size" of operand
// enumerate them (NB: align with processor definitions if possible)
enum z80_op_size_t
{
      OP_SIZE_WORD      // 0
    , OP_SIZE_BYTE      // 1
    , NUM_OP_SIZE
};


struct z80_mcode_t : tgt::tgt_mcode_t<z80_mcode_t, z80_stmt_t, error_msg>
{
    using BASE_NAME = KAS_STRING("Z80");

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

    // z80: base code & displacement interspersed in output
    template <typename ARGS_T> 
    void emit(core::core_emit&, ARGS_T&&, stmt_info_t const&) const;
    
    // support different branch sizes
    uint8_t calc_branch_mode(uint8_t size) const;
};

}
#endif

