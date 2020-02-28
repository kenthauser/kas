#ifndef KAS_Z80_Z80_MCODE_IMPL_H
#define KAS_Z80_Z80_MCODE_IMPL_H

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

#if 1
// determine size of immediate arg
uint8_t z80_mcode_t::sz(stmt_info_t info) const
{
    return 0;
}
#endif

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

template <typename ARGS_T>
void z80_mcode_t::emit(
                   core::emit_base& base
                 , ARGS_T&&    args
                 , stmt_info_t const& info
                 ) const
{
    // 0. get base machine code data
    auto machine_code = derived().code(info);
    auto code_p       = machine_code.data();

    // 1. apply args & emit relocs as required
    // now that have selected machine code match, there must be validator for each arg
    
    // Insert args into machine code "base" value
    // if base code has "relocation", emit it
    auto val_iter = vals().begin();
    unsigned n = 0;
    for (auto& arg : args)
    {
        auto val_p = &*val_iter++;
        if (!fmt().insert(n, code_p, arg, val_p))
            fmt().emit_reloc(n, base, code_p, arg, val_p);
        ++n;
    }
#if 0
    // 2. emit base code
    auto words = code_size()/sizeof(mcode_size_t);
    while (words--)
        base << *code_p++;

    // 3. emit arg information
    for (auto& arg : args)
        arg.emit(base, info);
   
    // XXX
    using expression::expr_fits;
#endif

    // 2. prefix (if defined) emitted first
    auto pfx = z80_arg_t::prefix;
    if (pfx)
        base << pfx;

    // 3. then first word of opcode
    base << *code_p;

    // 4. if prefix, must emit offset before anything else
    if (pfx)
    {
        // look for IX/IY arg
        for (auto const& arg : args)
        {
            switch (arg.mode())
            {
                // not index reg: continue looking
                default:
                    continue;

                // IX/IY w/o offset
                case MODE_REG_IX:
                case MODE_REG_IY:
                    break;      
                
                // IX/IY with zero offset
                case MODE_REG_INDIR_IX:
                case MODE_REG_INDIR_IY:
                    base << core::set_size(1) << 0;
                    break;
                
                // IX/IY with offset
                case MODE_REG_OFFSET_IX:
                case MODE_REG_OFFSET_IY:
                    base << core::set_size(1) << arg.expr();
                    break;
            }
            break;  // found IX/IY -> exit `for` loop
        }
    }

    // 5. now rest of opcode (z80 -> two bytes maximum)
    if (code_size() > sizeof(mcode_size_t))
        base << *++code_p;

#if 1
    // 6. emit arg information
    for (auto& arg : args)
        arg.emit(base, info.sz());
#else

    // emit additional arg data
    // hook into validators
    auto& val_c = defn().vals();
    auto  val_p = val_c.begin();

    // for `size` call
    auto fits = core::core_fits(dot_p);
    
    for (auto& arg : args)
    {
        op_size_t size;
#ifdef XXX
        // arg needs "size" to emit properly
        val_p->size(arg, sz(), fits, size);
        if (size())
            arg.emit(base, sz(), size());
#endif
        // next validator
        ++val_p; 
    } // for
#endif

}

}

#endif
