#ifndef KAS_M68K_M68K_ARG_EMIT_H
#define KAS_M68K_M68K_ARG_EMIT_H

#include "m68k_arg.h"
#include "m68k_mcode.h"
//#include "m68k_size_defn.h"
#include "kas_core/core_emit.h"

namespace kas::m68k
{
// size is from validator which finializes `modes` for arguments
void m68k_arg_t::emit(core::emit_base& base, uint8_t sz, unsigned size) const
{
    if (size == 0)
        return;             // nothing to emit

    uint8_t flt_size{};     // non-zero bit-format if emitting float

    switch (mode())
    {
        default:
            return;         // DONE

        // signed
        case MODE_ADDR_DISP:
        case MODE_PC_DISP:
        case MODE_MOVEP:
            size = 2;
            break;

        // unsigned
        case MODE_REGSET:
        case MODE_DIRECT_SHORT:
        case MODE_INDEX_BRIEF:
        case MODE_PC_INDEX_BRIEF:
            size = 2;
            break;

        // unsigned
        case MODE_DIRECT_LONG:
            size = 4;
            break;
        
        // immediate: store fixed & float formats
        case MODE_IMMED:
            size     = immed_info(sz).sz_bytes;
            break;

        // byte branch emitted as part of base word
        case MODE_BRANCH_BYTE:
            return;

        // word & long branch offsets
        case MODE_BRANCH_WORD:
        case MODE_BRANCH_LONG:
        {
            // word & long branches have displacments from displacement location
            if (mode() == MODE_BRANCH_WORD)
                base << core::set_size(2);
            else
                base << core::set_size(4);
                
            static constexpr core::core_reloc reloc { core::K_REL_ADD, 0, true };
            base << core::emit_reloc(reloc) << expr << 0;
            return;
        }

        case MODE_INDEX:
        case MODE_PC_INDEX:
            // special case INDEX
            {
                // 1) always emit extension word
                base << ext.hw_value();

                // 2) calculate size of inner word
                switch (ext.disp_size)
                {
                    case M_SIZE_ZERO:
                        size = 0;
                        break;
                    case M_SIZE_WORD:
                        size = 2;
                        break;
                    case M_SIZE_NONE:   // AUTO
                    case M_SIZE_LONG:
                        size = 4;
                        break;
                }
                
                // 3) emit inner word if needed
                if (size)
                    base << core::set_size(size) << expr;
                
                // 4) calculate size of outer word
                if (!ext.outer())
                    return;

                switch (ext.outer_size())
                {
                    case M_SIZE_ZERO:
                        size = 0;
                        break;
                    case M_SIZE_WORD:
                        size = 2;
                        break;
                    case M_SIZE_NONE:       // AUTO
                    case M_SIZE_LONG:
                        size = 4;
                        break;
                }
                
                // 5) emit outer word if needed
                if (size)
                    base << core::set_size(size) << outer;

                // 6) done with emit
                return;
            }          

            break;
    }

    // check for "byte" emit
    if (size == 1)
    {
        // byte immed actually causes word to be emitted
        // emit `expr` as 8-bit reloc with offset of 1 & not pc_rel
        // emit zero word as actual data
        static constexpr core::core_reloc reloc { core::K_REL_ADD, 8, false };
        base << core::emit_reloc(reloc, 0, 1) << expr;
        base << core::set_size(2) << 0;
    }
    else
        base << core::set_size(size) << expr;
}
}
#endif

