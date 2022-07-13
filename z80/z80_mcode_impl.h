#ifndef KAS_Z80_Z80_MCODE_IMPL_H
#define KAS_Z80_Z80_MCODE_IMPL_H

/////////////////////////////////////////////////////////////////////////
//
//  Emit z80 instruction: opcode & arguments
//
/////////////////////////////////////////////////////////////////////////


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

#include "z80_mcode.h"

#include "target/tgt_info_fn.h"
#include "target/tgt_opc_base.h"
#include "kas_core/core_emit.h"

namespace kas::z80
{

using expression::e_fixed_t;

void z80_opc_base::do_emit(core::core_emit& base
               , z80_mcode_t const& mcode
               , argv_t& args
               , stmt_info_t info) const
{
    // 0. get base machine code data
    auto machine_code = mcode.code(info);
    auto code_p       = machine_code.data();

    // 1. apply args & emit relocs as required
    // now that have selected machine code match, there must be validator for each arg
    
    // Insert args into machine code "base" value
    // if base code has "relocation", emit it
    auto val_iter = mcode.vals().begin();
    unsigned n = 0;
    for (auto& arg : args)
    {
        auto val_p = &*val_iter++;
        if (!mcode.fmt().insert(n, code_p, arg, val_p))
            mcode.fmt().emit_reloc(n, base, code_p, arg, val_p);
        ++n;
    }
    
    // 2. prefix (if defined) emitted first
    auto pfx = z80_arg_t::prefix;
    if (pfx)
        base << pfx;

    // 3. then first word of opcode
    base << *code_p;

    // 4. if prefix, must emit offset next
    if (pfx)
    {
        // look for IX/IY arg
        for (auto const& arg : args)
        {
            //std::cout << "z80_emit::arg = " << arg << std::endl;
            //std::cout << "z80_emit::mode = " << +arg.mode() << std::endl;
            switch (arg.mode())
            {
                // not index reg: continue looking
                default:
                    continue;

                // IX/IY w/o offset
                case MODE_REG_IX:
                case MODE_REG_IY:
                    break;      
                
                // IX/IY indirect with zero offset
                case MODE_REG_INDIR_IX:
                case MODE_REG_INDIR_IY:
                    base << core::set_size(1) << 0;
                    break;
                
                // IX/IY indirect with offset
                case MODE_REG_P_OFFSET_IX:
                case MODE_REG_P_OFFSET_IY:
                    base << core::set_size(1) << arg.expr;
                    break;
                case MODE_REG_M_OFFSET_IX:
                case MODE_REG_M_OFFSET_IY:
                {
                    // use KBFD to negate offset expression
                    // subtract `arg.expr` from base value of zero
                    static kbfd::kbfd_reloc neg_offset { kbfd::K_REL_SUB() };
                    base << core::set_size(1);
                    base << core::emit_reloc(neg_offset) << arg.expr << 0;
                    break;
                }
            }
            break;  // found IX/IY -> exit `for` loop
        }
    }

    // 5. now rest of opcode (z80 -> two bytes maximum)
    if (mcode.code_size() > sizeof(mcode_size_t))
        base << *++code_p;

    // 6. emit arg data
    auto sz = info.sz(mcode); 
    for (auto& arg : args)
        arg.emit(base, sz);
}
}

#endif
