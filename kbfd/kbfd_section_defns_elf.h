#ifndef KBFD_KBFD_SECTION_DEFNS_ELF_H
#define KBFD_KBFD_SECTION_DEFNS_ELF_H

#include "kbfd_section_defns.h"

namespace kbfd
{

struct kbfd_target_sections_elf : kbfd_target_sections
{
    using kbfd_target_sections::kbfd_target_sections;

    private:
        static constexpr kbfd_section_defn defns[] =
        {
              {".text",     SHT_PROGBITS,   SHF_ALLOC+SHF_EXECINSTR }
            , {".data",     SHT_PROGBITS,   SHF_ALLOC+SHF_WRITE  }
            , {".bss",      SHT_NOBITS,     SHF_ALLOC+SHF_WRITE  }
            , {".comment",  SHT_PROGBITS }
            , {".data1",    SHT_PROGBITS,   SHF_ALLOC+SHF_WRITE  }
            , {".debug",    SHT_PROGBITS }
            , {".fini",     SHT_PROGBITS,   SHF_ALLOC+SHF_EXECINSTR  }
            , {".fini_array", SHT_PROGBITS, SHF_ALLOC+SHF_WRITE  }
            , {".init",     SHT_PROGBITS,   SHF_ALLOC+SHF_EXECINSTR  }
            , {".init_array", SHT_PROGBITS, SHF_ALLOC+SHF_WRITE  }
            , {".line",     SHT_PROGBITS }
            , {".note",     SHT_NOTE }
            , {".preinit_array", SHT_PREINIT_ARRAY, SHF_ALLOC+SHF_WRITE  }
            , {".rodata",   SHT_PROGBITS,   SHF_ALLOC }
            , {".rodata1",  SHT_PROGBITS,   SHF_ALLOC }
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
};

}
#endif

