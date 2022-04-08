#ifndef KAS_ARM_ARM_MCODE_IMPL_H
#define KAS_ARM_ARM_MCODE_IMPL_H

// implement `arm_mcode_t` methods overriden in CRTP derived class

// Status bits in ARM:
//
// 1. condition-code: everywhere...
//
// 2. addressing mode 1: only S_BIT (and Immed_bit)
//     NB: 32-bit immed (means no U-bit)
//
// 3. addressing mode 2: P/U/B/W/L (P=0/W=1 -> T)
//
// 4. addressing mode 3: P/U/W/L. Also SH-> bits 6&5 differ on L/S bit
//
// 5. addressing mode 4: P/U/S/W/L: P/U set from SFX differ on L/S bit
//
// 6. addressing mode 5: (coprocessor): P/U/N/W/L (N sits at 22 (s-bit)
//                                                  and depends on co-processor)
//
// Status bits in T16:
//
// 1. ccode shifted 8 for branch. 
//




#include "arm_mcode.h"
#include "arm_info_impl.h"

namespace kas::arm
{

template <typename ARGS_T>
void arm_mcode_t::emit(core::core_emit& base
                     , ARGS_T&& args
                     , stmt_info_t const& info) const
{
    // 0. generate base machine code data
    auto machine_code = derived().code(info);
    auto code_p       = machine_code.data();

    // 1. apply args & emit relocs as required
    // NB: matching mcodes have a validator for each arg
    
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

    // 2. emit base code
    auto words = code_size()/sizeof(mcode_size_t);
    if (defn_arch() != 1)
    {
        for (auto end = code_p + words; code_p < end;)
        {
            // convert mcode_size_t (16-bits) to 32-bits
            uint32_t value = *code_p++ << 16;
            value |= *code_p++;
            base << value;
        }
    }
    else
    {
        for (auto end = code_p + words; code_p < end;)
        {
            base << *code_p++;
        }
    }

    // 3. emit arg information
    auto sz = info.sz(derived());
    for (auto& arg : args)
        arg.emit(base, sz);
}
}

#endif
