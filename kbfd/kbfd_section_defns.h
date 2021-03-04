#ifndef KBFD_KBFD_SECTION_DEFNS_H
#define KBFD_KBFD_SECTION_DEFNS_H

#include "kbfd_target_format.h"
#include "elf_common.h"

namespace kbfd
{
struct kbfd_section_defn
{
    const char *name;           // by convention: lower case
    kbfd_word   sh_type;        // holds SHT_* values
    kbfd_word   sh_flags;       // holds SHF_* values
};

struct kbfd_target_sections
{
    virtual ~kbfd_target_sections() = default;

    virtual auto get_initial_sections() const
            -> std::pair<kbfd_section_defn const *
                       , kbfd_section_defn const *>
    {
        static constexpr kbfd_section_defn initial[] =
            {   
                  {".text",     SHT_PROGBITS,   SHF_ALLOC+SHF_EXECINSTR }
                , {".data",     SHT_PROGBITS,   SHF_ALLOC+SHF_WRITE     }
                , {".bss",      SHT_NOBITS,     SHF_ALLOC+SHF_WRITE     }
            };

        return { std::begin(initial), std::end(initial) };
    }
    
    virtual auto get_all_sections() const
            -> std::pair<kbfd_section_defn const *
                       , kbfd_section_defn const *>
    {
        return get_initial_sections();
    }
    
    virtual auto get_initial() const
            -> kbfd_section_defn const *
    {
        return get_initial_sections().first;
    }

    virtual auto get_local_common() const
            -> kbfd_section_defn const *
    {
        auto [iter, end] = get_initial_sections();
        while (iter != end)
        {
            if (iter->sh_type == SHT_NOBITS)
                return iter;
            ++iter;
        }
        return {};
    }

    virtual auto get_section_defn(const char *name
                                , unsigned sub_section = {}) const
            -> std::pair<kbfd_section_defn const *, unsigned>
    {
        auto is_same = [](const char *lower, const char *mixed) -> bool
        {
            while (*lower)
                if (*lower++ != std::tolower(*mixed++))
                    return false;

            return !*mixed;     // true if end-of-string
        };

        auto [iter, end] = get_all_sections();
        while (iter != end)
            if (is_same(iter->name, name))
                return { iter, sub_section };
        return {};
    }
};

}
#endif
