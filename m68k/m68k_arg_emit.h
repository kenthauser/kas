#ifndef KAS_M68K_M68K_ARG_EMIT_H
#define KAS_M68K_M68K_ARG_EMIT_H

#include "m68k_arg.h"
#include "m68k_mcode.h"
//#include "m68k_size_defn.h"
#include "kas_core/core_emit.h"

namespace kas::m68k
{
// size is from validator which finializes `modes` for arguments
void m68k_arg_t::emit(m68k_mcode_t const& mcode, core::emit_base& base, unsigned size) const
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
        case MODE_IMMED_LONG:
        case MODE_IMMED_SINGLE:
        case MODE_IMMED_XTND:
        case MODE_IMMED_PACKED:
        case MODE_IMMED_WORD:
        case MODE_IMMED_DOUBLE:
        case MODE_IMMED_BYTE:
            size     = immed_info(mcode.sz()).sz_bytes;
            flt_size = immed_info(mcode.sz()).flt_fmt;    // XXX?
            break;

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
    // XXX Really want "byte-sized" relocation
    if (size == 1)
    {
        // byte immed acutally causes word to be emitted
        base << core::set_size(1) << 0;
        base << core::set_size(1) << expr;
    }
    else
        base << core::set_size(size) << expr;
}
}
#endif

