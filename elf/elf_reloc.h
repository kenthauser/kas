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
    static constexpr auto RFLAGS_PC_REL = 1;
    
    kas_reloc() = default;      // NB: not constexpr

    constexpr kas_reloc(uint8_t action, uint8_t bits = 0, uint8_t pc_rel = false)
        : action(action)
        , bits(bits)
        , flags (pc_rel ? RFLAGS_PC_REL : 0)
        {}
#if 1
    // allow `action` to be defined as `type`
    template <typename ACTION, typename...Ts
            , typename = std::enable_if_t<!std::is_integral_v<ACTION>>>
    constexpr kas_reloc(ACTION const& action, Ts&&...args)
        : kas_reloc(action_index(action), std::forward<Ts>(args)...) {}
    template <typename ACTION> 
    constexpr uint16_t action_index(ACTION const&) const
    {
        return meta::find_index<reloc_ops_v,ACTION>::value + 1;
    }
#endif

    // allow 'action' to be defined as `string`
    template <typename...Ts>
    kas_reloc(const char *action, Ts&&...args)
        : kas_reloc(reloc_get_action(action), std::forward<Ts>(args)...) 
        {}


    const reloc_op_fns& get() const
    {
        if (action)
            return *reloc_ops_p[action-1];
        throw std::runtime_error{"kas_reloc::get: undefined action"};
    }

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
    uint8_t     num;        // well-known-number for interchange format
};

// hold info about target relocations
struct elf_reloc_t
{
    // cononical constructor
    constexpr elf_reloc_t(kas_reloc_info const *reloc_info
                        , reloc_op_fns   const *const *const reloc_ops
                        , std::size_t num_info
                        , std::size_t num_ops
                        , kas_rela    rela = {}
                        )
        : reloc_info (reloc_info)
        , ops        (reloc_ops)
        , num_info   (num_info)
        , num_ops    (num_ops)
        , rela       (rela)
        {}

    // pass arrays as arrays...
    template <size_t N, size_t R, typename...Ts>
    constexpr elf_reloc_t(kas_reloc_info const (&reloc_info)[N]
                        , reloc_op_fns   const * const(&reloc_ops)[R]
                        , Ts... args)
                : elf_reloc_t(reloc_info, reloc_ops, N, R, std::forward<Ts>(args)...)
                {}

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
#if 0
        static std::map<uint8_t, reloc_op_fns> fn_map{reloc_ops_p, reloc_ops_p+num_ops};
        return fn_map.at(reloc.action);
#else
        if (reloc.action >= num_ops)
            throw std::runtime_error{"elf_reloc_t::get_ops: invalid code"};
        auto p = ops[reloc.action];
        if (!p) p = ops[0];      // default to "none"
        return *p;
#endif
    }

    // method for `ostream`: direct search, no `map`
    const char *get_info(uint8_t r_num, kas_reloc_info const **info_p = {}) const
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
    kas_reloc_info const * const  reloc_info;
    reloc_op_fns   const * const *ops;
    const std::size_t num_info, num_ops;
    const kas_rela    rela;
};

}

#endif
