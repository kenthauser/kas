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
    // use default ctors
    using base_t::base_t;

    // prefix is part of `base` machine code size calculation
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

#if 0
// XXX must move to impl file
template <typename ARGS_T>
void z80_mcode_t::emit(
                   core::emit_base& base
                 , mcode_size_t *op_p
                 , ARGS_T&&    args
                 , core::core_expr_dot const *dot_p
                 ) const
{
    // prefix first...
    uint8_t pfx = arg_t::prefix;
    
    if (pfx)
        base << pfx;

    // then first word of opcode
    base << *op_p;

    // if offset, must emit offset next
    if (pfx)
    {
        for (auto& arg : args)
        {
            switch (arg.mode())
            {
                default:
                    continue;   // next arg

                case MODE_REG_OFFSET_IX:
                case MODE_REG_OFFSET_IY:
                    base << core::set_size(1) << arg.expr;
                    break;      // exit switch
            }
            break;  // exit for loop
        }
    }

    // now rest of opcode
    if (code_size() > 1)
        base << *++op_p;

    // for `size` call
    auto fits = core::core_fits(dot_p);
    
    // hook into validators
    auto& val_c = defn().val_c();
    auto  val_p = val_c.begin();

    // emit additional arg data
    for (auto& arg : args)
    {
        op_size_t size;

        // arg needs "size" to emit properly
        val_p->size(arg, fits, size);
        if (size())
            arg.emit(base, size());

        // next validator
        ++val_p; 
    } // for
}
#endif

}
#endif

