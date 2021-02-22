#ifndef KBFD_KBFD_TARGET_RELOC_H
#define KBFD_KBFD_TARGET_RELOC_H

#include "kbfd_reloc.h"
#include <map>

namespace kbfd
{

struct kbfd_target_reloc
{
    using index_t = uint8_t;

    // lookup `reloc_action` using `type` not name for constexpr
    template <typename ACTION, typename...Args>
    constexpr kbfd_target_reloc(uint8_t num, const char *name
                            , ACTION&& action, Args&&...args)
        : num(num)
        , name(name)
        , reloc(reloc_action::as_action(action), args...)
          {}

    const char *name;       // name to display for decode
    kbfd_reloc  reloc;      // use `key()` to find operations
    index_t     num;        // well-known-number for interchange format
};
#if 0
// hold info about target relocations
struct elf_reloc_t
{
    // cononical constructor
    constexpr elf_reloc_t(kbfd_target_reloc const *reloc_info
                        , std::size_t num_info
                        , kbfd_rela   rela = {}
                        )
        : reloc_info (reloc_info)
        , num_info   (num_info)
        , rela       (rela)
        {}

    // pass arrays as arrays...
    template <size_t N, typename...Ts>
    constexpr elf_reloc_t(kbfd_target_reloc const (&reloc_info)[N]
                        , Ts... args)
                : elf_reloc_t(reloc_info, N, std::forward<Ts>(args)...)
                {}

    // XXX temp implementations using std::map
    // XXX only allow 1 map per compliation
    const char *get_info(kbfd_reloc const& reloc, kbfd_target_reloc const **info_p) const
    {
#if 0
        using key_t = decltype(reloc.key());
        using map_t = std::map<key_t, kbfd_target_reloc const *>;

        static map_t *map_p;
        if (!map_p)
        {
            map_p = new map_t;
            auto p = reloc_info;
            for (int i = 0; i < num_info; ++i, ++p)
            {
                auto [_, insert] = map_p->emplace(p->reloc.key(), p);
            }
        }

        auto iter = map_p->find(reloc.key());
        if (iter == map_p->end())
            return "invalid relocation";
        *info_p = iter->second;
#endif
        return {};
    }

    // method for `ostream`: direct search, no `map`
    const char *get_info(uint8_t r_num, kbfd_target_reloc const **info_p = {}) const
    {
        auto p = reloc_info;
        for (auto n = 0; n < num_info; ++n, ++p)
            if (p->num == r_num)
            {
                if (info_p)
                    (*info_p) = p;
                return p->name;
            }
        return "invalid relocation";
    }

private:
    kbfd_target_reloc const * const reloc_info;
    const uint8_t   num_info;
    const kbfd_rela rela;
};
#endif
}

#endif
