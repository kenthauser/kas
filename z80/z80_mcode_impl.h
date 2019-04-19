#ifndef KAS_Z80_Z80_MCODE_IMPL_H
#define KAS_Z80_Z80_MCODE_IMPL_H

//
// Z80 opcode emit rules:
//
// 1. If arg prefix set, emit prefix (byte)
//
// 2. Emit first byte of opcode
//
// 3. If prefix, emit displacement byte (if offset format)
//
// 4. If remaining byte of opcode, emit.
//
// 5. Emit byte or word arg expr
//

#include "target/tgt_mcode_defn.h"
#include "kas_core/core_emit.h"
#include "expr/expr_fits.h"

namespace kas::z80
{
/////////////////////////////////////////////////////////////////////////
//
//  Emit z80 instruction: opcode & arguments
//
/////////////////////////////////////////////////////////////////////////

using expression::e_fixed_t;


template <typename ARGS_T>
void z80_mcode_t::emit(
                   core::emit_base& base
                 , mcode_size_t *op_p
                 , ARGS_T&&    args
                 , core::core_expr_dot const *dot_p) const
{
    using expression::expr_fits;
   
    // prefix (if defined) emitted first
    uint8_t pfx = z80_arg_t::prefix;
    if (pfx)
        base << pfx;

    // then first word of opcode
    base << *op_p;

    // if prefix, must emit offset before anything else
    if (pfx)
    {
        // look for IX/IY arg
        for (auto const& arg : args)
        {
            switch (arg.mode())
            {
                default:
                    continue;
                case MODE_REG_IX:
                case MODE_REG_IY:
                    // IX/IY w/o offset
                    break;      
                case MODE_REG_INDIR_IX:
                case MODE_REG_INDIR_IY:
                    base << core::set_size(1) << 0;
                    break;
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

    // emit additional arg data
    // hook into validators
    auto& val_c = defn().vals();
    auto  val_p = val_c.begin();

    // for `size` call
    auto fits = core::core_fits(dot_p);
    
    for (auto& arg : args)
    {
        op_size_t size;

        // arg needs "size" to emit properly
        val_p->size(arg, sz(), fits, size);
        if (size())
            arg.emit(base, sz(), size());

        // next validator
        ++val_p; 
    } // for
}

}

#endif
