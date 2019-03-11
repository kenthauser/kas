#ifndef KAS_Z80_Z80_MCODE_H
#define KAS_Z80_Z80_MCODE_H

#include "z80_stmt.h"
#include "target/tgt_mcode.h"

#include "kas_core/core_emit.h"
#include "kas_core/core_fits.h"

// instruction per-size run-time object
// NB: not allocated if info->hw_tst fails, unless no
// other insn with name allocated...

namespace kas::z80
{

// override defaults for various sizes
struct z80_mcode_size_t : tgt::tgt_mcode_size_t
{
    using mcode_size_t = uint8_t;
};


struct z80_mcode_t : tgt::tgt_mcode_t<z80_mcode_t, z80_stmt_t, error_msg, z80_mcode_size_t>
{
    using BASE_NAME = KAS_STRING("Z80");

    // use default ctors
    using base_t::base_t;

    // prefix is part of `base` machine code size calculation
    // not part of `arg` size calculation
    auto base_size() const
    {
        // NB: sizeof(mcode_size_t) == 1
        return code_size() + (arg_t::prefix != 0);
    }

    // z80: base code & displacement interspersed in output
    template <typename ARGS_T> 
    void emit(core::emit_base&
            , mcode_size_t *
            , ARGS_T&&
            , core::core_expr_dot const*
            ) const;
};


}
#endif

