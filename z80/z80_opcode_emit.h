#ifndef KAS_Z80_Z80_OPCODE_EMIT_H
#define KAS_Z80_Z80_OPCODE_EMIT_H

//
// Z80 opcode emit rules:
//
// 1. Search args for prefix. If found, emit (byte)
//
// 2. If prefix, emit first word of opcode
//
// 3. If prefix, emit displacement (if any)
//
// 4. If remaining byte of opcode, emit.
//
// 5. Emit byte or word arg expr
//


#include "z80_insn_types.h"
//#include "z80_size_defn.h"
//#include "z80_format_float.h"

#include "kas_core/core_emit.h"
#include "expr/expr_fits.h"

namespace kas::z80::opc
{
/////////////////////////////////////////////////////////////////////////
//
//  Emit z80 instruction: opcode & arguments
//
/////////////////////////////////////////////////////////////////////////

using z80_word_size_t = uint16_t;
using expression::e_fixed_t;


template <typename ARGS_T>
void z80_opcode_t::emit(
                   core::emit_base& base
                 , mcode_size_t *op_p
                 , ARGS_T&&    args
                 , core::core_expr_dot const *dot_p) const
{
    using expression::expr_fits;
   
    // prefix first...
    uint8_t pfx = z80_arg_t::prefix;
    if (pfx)
        base << pfx;

    // then first word of opcode
    base << core::set_size(1) << *op_p;

    // if prefix, must emit offset before anything else
    if (pfx)
    {
        for (auto& arg : args)
        {
            switch (arg.mode())
            {
                default:
                    continue;
                case MODE_REG_OFFSET_IX:
                case MODE_REG_OFFSET_IY:
                    base << core::set_size(1) << arg.expr;
                    break;      // exit switch
            }
            break;  // exit for loop
        }
    }

    // now rest of opcode
    if (opc_long)
        base << core::set_size(1) << *++op_p;

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

}

#endif
