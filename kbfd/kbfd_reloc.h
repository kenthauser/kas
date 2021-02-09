#ifndef KBFD_KBFD_RELOC_H
#define KBFD_KBFD_RELOC_H

#include "kbfd_reloc_ops.h"
#include <map>

namespace kbfd
{

enum class kbfd_rela
{
      RELA_ALLOW        // use RELA if value doesn't fit
    , RELA_NONE         // don't allow RELA
    , RELA_REQUIRE      // always use RELA
};


// Describe generic KBFD relocation operation
// NB: `action` can identify both generic and target specific operations
struct kbfd_reloc
{
    using hash_t = std::tuple<reloc_action, uint8_t, uint8_t>;

    static constexpr auto RFLAGS_PC_REL = 1;
    
    kbfd_reloc() = default;      // NB: not constexpr

    constexpr kbfd_reloc(reloc_action action
                       , uint8_t bits = 0
                       , uint8_t pc_rel = false
                       )
        : action(action)
        , bits(bits)
        , flags (pc_rel ? RFLAGS_PC_REL : 0)
        {}

    // allow 'action' to be defined as `string`
    template <typename...Ts>
    kbfd_reloc(const char *action, Ts&&...args)
        : kbfd_reloc(reloc_action(action), std::forward<Ts>(args)...) 
        {}

    const reloc_op_fns& get() const
    {
        return action.get();

        throw std::runtime_error{"kbfd_reloc::get: undefined action"};
    }

    // generate `std::map` key (from 3 8-bit values)
    constexpr hash_t key() const
    {
       return std::make_tuple(action, bits, flags);
    }

    // NB: defined in `kbfd_format_impl.h`
    friend std::ostream& operator<<(std::ostream&, kbfd_reloc const&);

    reloc_action action {};
    uint8_t      bits   {};
    uint8_t      flags  {};
};


}

#endif
