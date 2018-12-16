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
                 , uint16_t *op_p
                 , ARGS_T&&    args
                 , core::core_expr_dot const& dot) const
{
    using expression::expr_fits;
    
    base << *op_p;
    if (opc_long)
        base << *++op_p;

    // emit additional arg data
    for (auto& arg : args)
    {
        switch (arg.mode) {
            default:
                 break;
#if 0
            // M_SIZE_WORD & M_SIZE_SWORD emit identically
            case MODE_ADDR_DISP:
            case MODE_DIRECT_ALTER:     // XXX should not happen?
            case MODE_DIRECT_SHORT:
            case MODE_INDEX_BRIEF:
            case MODE_MOVEP:
            case MODE_PC_DISP:
                base << core::set_size(2) << arg.disp;
                break;
            
            case MODE_DIRECT_LONG:
                base << core::set_size(4) << arg.disp;
                break;
            
            case MODE_DIRECT:
                // always first extenion word.
                // PC is address of first extension word.
                // XXX may need to adjust for 32-bit opcode
                base << core::emit_disp(dot, 2, 2) << arg.disp;
                break;

            case MODE_IMMED:
            case MODE_IMMED_LONG:       // 0
            case MODE_IMMED_SINGLE:     // 1
            case MODE_IMMED_XTND:       // 2
            case MODE_IMMED_PACKED:     // 3
            case MODE_IMMED_WORD:       // 4
            case MODE_IMMED_DOUBLE:     // 5
            case MODE_IMMED_BYTE:       // 6
                // calculate `insn_sz` from mode
                emit_immed(sz(), arg.disp);
                break;

            case MODE_INDEX:
            case MODE_PC_INDEX:
                emit_index(arg.ext, arg.disp, arg.outer);
                break;
#endif
        } // switch
    } // for
}

}

#endif
