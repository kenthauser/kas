#ifndef KAS_M68K_M68K_ELF_H
#define KAS_M68K_M68K_ELF_H

#include "elf/emit_elf.h"
#include "binutil/include/m68k.h"

namespace kas::m68k
{

struct m68k_elf : elf::emit_elf
{
    using namespace elf;

    m68k_elf() : emit_elf(m68k_reloc_type, ELFCLASS32, ELFDATA2MSB, EM_68K)
    {
        // base.set_machine(xxx)

    }

    // convert generic ELF reloc to `R_68K_*` reloc
    static uint32_t m68k_reloc_type(RELOC_TYPE t, unsigned n)
    {
        static const uint32_t reloc_t[][4] = {
              { R_68K_NONE, R_68K_8, R_68K_16, R_68K_32 }
            , { R_68K_NONE, R_68K_PC8, R_68K_PC16, R_68K_PC32 }
            , { R_68K_NONE, R_68K_GOT8, R_68K_GOT16, R_68K_GOT32 }
            , { R_68K_NONE, R_68K_GOT8O, R_68K_GOT16O, R_68K_GOT32O }
            , { R_68K_NONE, R_68K_PLT8, R_68K_PLT16, R_68K_PLT32 }
            , { R_68K_NONE, R_68K_PLT8O, R_68K_PLT16O, R_68K_PLT32O }
            };

        // XXX can rework with c++17 std::extent in the future
        if (n > 3)
            return R_68K_NONE;
        auto p = &reloc_t[t][n];
        if (p >= std::end(reloc_t))
            return R_68K_NONE;
        return *p;
    }
};


}



#endif