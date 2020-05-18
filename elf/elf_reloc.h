#ifndef KAS_ELF_ELF_RELOC_H
#define KAS_ELF_ELF_RELOC_H

#include "elf_reloc_ops.h"

namespace kas::elf
{

enum class kas_rela
{
      RELA_ALLOW        // use RELA if value doesn't fit
    , RELA_NONE         // don't allow RELA
    , RELA_REQUIRE      // always use RELA
};


// Describe generic KAS relocation operation
// NB: `action` can identify both generic and target specific operations
struct kas_reloc
{
    // 
    static constexpr auto RFLAGS_PC_REL = 1;
    
    // XXX req'd for `add_reloc` default ctor
    kas_reloc() = default;

    constexpr kas_reloc(uint8_t action, uint8_t bits = 0, uint8_t pc_rel = false)
        : action(action)
        , bits(bits)
        , flags ( pc_rel ? RFLAGS_PC_REL : 0
                )
        {}
    
    // generate `std::map` key (from 3 8-bit values)
    constexpr auto key() const
    {
       return std::make_tuple(action, bits, flags);
    }

    uint8_t action {};
    uint8_t bits   {};
    uint8_t flags  {};
};


struct kas_reloc_info
{
    constexpr kas_reloc_info(uint8_t num, const char *name, kas_reloc reloc)
        : num(num), name(name), reloc(reloc) {}

    const char *name;       // name to display for decode
    kas_reloc   reloc;      // use `key()` to find operations
    uint8_t     num;        // magic number for object code
};

// hold info about target relocations
struct elf_reloc_t
{
    constexpr elf_reloc_t(kas_reloc_info const *reloc_info
              , reloc_ops_t   const *reloc_ops
              , std::size_t num_info
              , std::size_t num_ops
              , kas_rela rela = {}
              )
        : reloc_info (reloc_info)
        , ops        (reloc_ops)
        , num_info   (num_info)
        , num_ops    (num_ops)
        , rela       (rela)
        {}

    // accept array of info with no ops
    template <std::size_t NUM_INFO>
    constexpr elf_reloc_t(kas_reloc_info const reloc_info[NUM_INFO], kas_rela rela = {})
        : elf_reloc_t(reloc_info, nullptr, NUM_INFO, 0, rela) {}

    // accept arrays
    template <std::size_t NUM_INFO, std::size_t NUM_OPS>
    constexpr elf_reloc_t(kas_reloc_info const reloc_info[NUM_INFO]
              , reloc_ops_t   const reloc_ops[NUM_OPS]
              , kas_rela rela = {}
              )
        : elf_reloc_t(reloc_info, reloc_ops, NUM_INFO, NUM_OPS, rela) {}

    // XXX temp implementations using std::map
    // XXX only allow 1 map per compliation
    const char *get_info(kas_reloc const& reloc, kas_reloc_info const **info_p) const
    {
        using key_t = decltype(reloc.key());
        using map_t = std::map<key_t, kas_reloc_info const *>;

        static map_t *map_p;
        if (!map_p)
        {
            map_p = new map_t;
            auto p = reloc_info;
            for (int i = 0; i < num_info; ++i, ++p)
            {
                auto [_, insert] = map_p->emplace(p->reloc.key(), p);
            #if 0
            // XXX relocations can have multiple names. only finds first
                if (!insert)
                    throw std::logic_error("duplicate reloc" + std::string(p->name));
            #endif
            }
        }

        auto iter = map_p->find(reloc.key());
        if (iter == map_p->end())
            return "invalid relocation";
        *info_p = iter->second;
        return {};
    }

    reloc_op_fns const& get_ops(kas_reloc const& reloc) const
    {
        static std::map<uint8_t, reloc_op_fns> fn_map{reloc_ops, reloc_ops+num_ops};
        return fn_map.at(reloc.action);
    }

private:
    kas_reloc_info const * const reloc_info;
    reloc_ops_t    const * const ops;
    const std::size_t num_info, num_ops;
    kas_rela        rela;
};

}

#endif