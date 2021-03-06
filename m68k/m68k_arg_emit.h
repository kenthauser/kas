#ifndef KAS_M68K_M68K_ARG_EMIT_H
#define KAS_M68K_M68K_ARG_EMIT_H

#include "m68k_arg.h"
#include "m68k_mcode.h"
#include "kas_core/core_emit.h"

namespace kas::m68k
{

void m68k_arg_t::emit(core::core_emit& base, uint8_t sz)
{
    int size = 0;

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
        case MODE_DIRECT_SHORT:
            size = 2;
            break;

        // unsigned
        case MODE_DIRECT:
        case MODE_DIRECT_ALTER:
        case MODE_DIRECT_LONG:
            size = 4;
            break;
        
        // immediate: emit fixed & float formats
        case MODE_IMMEDIATE:
            if (sz == OP_SIZE_BYTE)
            {
                std::cout << "m68k_arg_emit::immed_byte" << std::endl;
                // emit 16-bit value with 8 LSBs holding value
                static kbfd::kbfd_reloc reloc { kbfd::K_REL_ADD(), 8 };
                base << core::set_size(2);        // m68k size is 16-bit
                if (!expr.empty())
                    base << core::emit_reloc(reloc, {}, 0, 1) << expr;
                base << 0;
            }
            else
                return emit_immed(base, sz);

        // byte branch emitted as part of base word
        case MODE_BRANCH_BYTE:
            return;

        // word & long branch offsets
        // also direct xlated into PC_REL
        case MODE_DIRECT_PCREL:
        case MODE_BRANCH_WORD:
        case MODE_BRANCH_LONG:
        {
            // word & long branches have displacments from displacement location
            if (mode() == MODE_BRANCH_LONG)
                size = 4;
            else
                size = 2;

            // width of zero implies width is picked up from `set_size()`
            static kbfd::kbfd_reloc reloc { kbfd::K_REL_ADD(), 0, true };
            base << core::set_size(size) << core::emit_reloc(reloc) << expr << 0;
            return;
        }

        // xlate MODE_ADDR_DISP_LONG -> MODE_INDEX
        case MODE_ADDR_DISP_LONG:
            set_mode(MODE_INDEX);
            ext.disp_size = M_SIZE_LONG;
            // FALLSTHRU

        // index: use `type` method
        case MODE_INDEX:
        case MODE_PC_INDEX:
            return ext.emit(base, *this, sz);
    }

    base << core::set_size(size) << expr;
}
}
#endif


