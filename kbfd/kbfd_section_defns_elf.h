#ifndef KBFD_KBFD_SECTION_DEFNS_ELF_H
#define KBFD_KBFD_SECTION_DEFNS_ELF_H

// declare the "special sections" from
// "Linux Standard Base Core Specification, Generic Part"
// "LSB Core - Generic 5.0 Edition"

#include "kbfd_section_defns.h"

namespace kbfd
{

struct kbfd_target_sections_elf : kbfd_target_sections
{
    using kbfd_target_sections::kbfd_target_sections;

    private:
        static constexpr kbfd_section_defn defns[] =
        {
            // first three are "initial sections"
              {".text",     SHT_PROGBITS,   SHF_ALLOC+SHF_EXECINSTR }
            , {".data",     SHT_PROGBITS,   SHF_ALLOC+SHF_WRITE  }
            , {".bss",      SHT_NOBITS,     SHF_ALLOC+SHF_WRITE  }
          
            // the others happen to be in alphabetical order
            , {".comment",  SHT_PROGBITS }
            , {".data1",    SHT_PROGBITS,   SHF_ALLOC+SHF_WRITE  }
            , {".debug",    SHT_PROGBITS }
            , {".dynamic",  SHT_DYNAMIC,    SHF_ALLOC+SHF_WRITE }
            , {".dynstr",   SHT_STRTAB,     SHF_ALLOC }
            , {".dynsym",   SHT_DYNSYM,     SHF_ALLOC }
            , {".fini",     SHT_PROGBITS,   SHF_ALLOC+SHF_EXECINSTR  }
            , {".fini_array", SHT_PROGBITS, SHF_ALLOC+SHF_WRITE  }
            , {".hash",     SHT_HASH,       SHF_ALLOC }
            , {".init",     SHT_PROGBITS,   SHF_ALLOC+SHF_EXECINSTR  }
            , {".init_array", SHT_PROGBITS, SHF_ALLOC+SHF_WRITE  }
            , {".interp",   SHT_PROGBITS,   SHF_ALLOC }
            , {".line",     SHT_PROGBITS }
            , {".note",     SHT_NOTE }
            , {".preinit_array", SHT_PREINIT_ARRAY, SHF_ALLOC+SHF_WRITE  }
            , {".rodata",   SHT_PROGBITS,   SHF_ALLOC+SHF_MERGE+SHF_STRINGS }
            , {".rodata1",  SHT_PROGBITS,   SHF_ALLOC+SHF_MERGE+SHF_STRINGS }
            , {".shstrtab", SHT_STRTAB }
            , {".strtab",   SHT_STRTAB,     SHF_ALLOC }
            , {".symtab",   SHT_SYMTAB,     SHF_ALLOC }
            , {".tbss",     SHT_NOBITS,     SHF_ALLOC+SHF_WRITE+SHF_TLS  }
            , {".tdata",    SHT_PROGBITS,   SHF_ALLOC+SHF_WRITE+SHF_TLS  }
            , {".tdata1",   SHT_PROGBITS,   SHF_ALLOC+SHF_WRITE+SHF_TLS  }
        };

    public:
        auto get_all_sections() const 
            -> std::pair<kbfd_section_defn const *
                       , kbfd_section_defn const *> override
        {
            return { std::begin(defns), std::end(defns) };
        }
    
        auto get_initial_sections() const
            -> std::pair<kbfd_section_defn const *
                       , kbfd_section_defn const *> override
        {
            // first three are "initial"
            return { std::begin(defns), std::begin(defns) + 3 };
        }

};

}
#endif

