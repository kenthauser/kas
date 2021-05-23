#ifndef KBFD_KBFD_RELOC_H
#define KBFD_KBFD_RELOC_H

#include "kbfd_reloc_ops.h"

namespace kbfd
{

// Describe generic KBFD relocation operation
// NB: `action` can identify both generic and target specific operations
struct kbfd_reloc
{
    using key_t = uint32_t;
static constexpr auto RFLAGS_DEPR   = 0x80;
    static constexpr auto RFLAGS_PC_REL = 0x40;
    
    kbfd_reloc() = default;      // NB: not constexpr

    constexpr kbfd_reloc(reloc_action action
                       , uint8_t bits = 0
                       , uint8_t pc_rel = false
                       , uint8_t flags = 0
                       )
        : action(action)
        , bits(bits)
        , flags (pc_rel ? RFLAGS_PC_REL : 0)
        {}

    // XXX
    template <typename...Ts>
    constexpr kbfd_reloc(reloc_action action, Ts&&...)
        : action(action) {}

    // allow 'action' to be defined as `string`
    template <typename...Ts>
    kbfd_reloc(const char *action, Ts&&...args)
        : kbfd_reloc(reloc_action(action), std::forward<Ts>(args)...) 
        {}

    // operations to modify relocation
    void default_width(uint8_t bytes)
    {
        if (!bits)
            bits = bytes * 8;
    }

    // manipulate flags
    bool has  (uint8_t flag) const { return flags  &  flag; }
    void clear(uint8_t flag)       {        flags &=~ flag; }
    void set  (uint8_t flag)       {        flags |=  flag; }

    // retrieve action operations
    const reloc_op_fns& get() const
    {
        return action.get();

        throw std::runtime_error{"kbfd_reloc::get: undefined action"};
    }

    // generate `std::map` key (from 3 8-bit values)
    constexpr key_t key() const
    {
       return (action.key() << 16) | (bits << 8) | flags;
    }

    // NB: defined in `kbfd_format_impl.h`
    friend std::ostream& operator<<(std::ostream&, kbfd_reloc const&);

    reloc_action action {};
    uint8_t      bits   {};
    uint8_t      flags  {};
};


}

#endif
