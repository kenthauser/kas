#ifndef KBFD_KBFD_RELOC_H
#define KBFD_KBFD_RELOC_H

#include "kbfd_reloc_ops.h"

namespace kbfd
{
// Describe generic KBFD relocation operation
// NB: `action` can identify both generic and target specific operations
struct kbfd_reloc
{
    using key_t   = uint32_t;
    using flags_t = uint16_t;

    // NB: for _SIZE_ values: bytes = 1 << (_SIZE_ - 1);
    static constexpr auto RFLAGS_SIZE_64    = 4;
    static constexpr auto RFLAGS_SIZE_32    = 3; 
    static constexpr auto RFLAGS_SIZE_16    = 2; 
    static constexpr auto RFLAGS_SIZE_8     = 1; 
    static constexpr auto RFLAGS_SIZE_0     = 0;  
    static constexpr auto RFLAGS_SIZE_MASK  = 0x7;
   
    // generic flags (lower 8 bits)
    static constexpr auto RFLAGS_DEPR       = 0x80;
    static constexpr auto RFLAGS_PC_REL     = 0x40;
    static constexpr auto RFLAGS_SB_REL     = 0x20;
    static constexpr auto RFLAGS_TLS        = 0x10;

    // target flags (upper 8 bits)
    // XXX need to figure way to generalize
    static constexpr auto RFLAGS_ARM_G_MASK = 0x0300;
    static constexpr auto RFLAGS_ARM_G0     = 0x0000;
    static constexpr auto RFLAGS_ARM_G1     = 0x0100;
    static constexpr auto RFLAGS_ARM_G2     = 0x0200;
    static constexpr auto RFLAGS_ARM_G3     = 0x0300;
    static constexpr auto RFLAGS_ARM_NC     = 0x0400;

    kbfd_reloc() = default;      // NB: not constexpr

    // actual constructor
    // NB: `bits` & `pc_rel` get separate args as they are most common flags 
    template <typename...Ts>
    constexpr kbfd_reloc(reloc_action action
                       , uint8_t bits   = 0
                       , uint8_t pc_rel = false
                       , Ts&&... _flags
                       )
        : action(action)
        , flags ((init_width_pcrel(bits, pc_rel) | ... | _flags))
        {}

    // allow 'action' to be defined as `string`
    // NB: used by `kbfd_target_reloc` constructor in definitions
    template <typename...Ts>
    kbfd_reloc(const char *action, Ts&&...args)
        : kbfd_reloc(reloc_action(action), std::forward<Ts>(args)...) 
        {}

    static constexpr flags_t init_width_pcrel(uint8_t bits, uint8_t pc_rel)
    {
        return get_width(bits) | (pc_rel ? RFLAGS_PC_REL : 0);
    }

    static constexpr flags_t get_width(uint8_t bits)
    {
        switch (bits)
        {
            case 64: return RFLAGS_SIZE_64;
            case 32: return RFLAGS_SIZE_32;
            case 16: return RFLAGS_SIZE_16;
            case  8: return RFLAGS_SIZE_8;

            default: return RFLAGS_SIZE_0;
        };
    }

    // operations to modify relocation
    void default_width(uint8_t bytes)
    {
        // if size not set, update to correspond to `bytes`
        if (!(flags & RFLAGS_SIZE_MASK))
            set_width(bytes * 8);
    }

    // manipulate reloc flags (NB: manipulators assume flag is single bit)
    bool has  (flags_t flag) const { return flags &   flag; }
    void clear(flags_t flag)       {        flags &=~ flag; }
    void set  (flags_t flag)       {        flags |=  flag; }

    void set_width(uint8_t bits)
    {
        flags = (flags &~ RFLAGS_SIZE_MASK) | get_width(bits);
    }

    // return size of reloc in bytes
    constexpr uint8_t bytes() const
    {
        auto size = flags & RFLAGS_SIZE_MASK;
        if (size) return (1 << (size - 1));
        return 0;
    }

    constexpr uint8_t bits() const
    {
        return bytes() * 8;
    }

    // retrieve action operations
    auto& get() const
    {
        return action.get();
    }

    // generate `std::map` key (from 2 16-bit values)
    constexpr key_t key() const
    {
       return (action.key() << 16) | flags;
    }

    // NB: defined in `kbfd_format_impl.h`
    friend std::ostream& operator<<(std::ostream&, kbfd_reloc const&);

    reloc_action action {};
    flags_t      flags  {};
};


}

#endif
