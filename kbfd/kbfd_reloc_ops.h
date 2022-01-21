#ifndef KBFD_KBFD_RELOC_OPS_H
#define KBFD_KBFD_RELOC_OPS_H

#include "kbfd.h"

namespace kbfd
{

// kas_defn declaration for `reloc_ops` definitions
template <typename = void> struct reloc_ops_v : meta::list<> {};

// format each "defn" as "list" as required for `init_from_list`
template <typename NAME, typename OP>// = reloc_op_fns, int...Ts>
using KBFD_ACTION = meta::list<NAME, OP>;//, NAME, meta::int_<Ts>...>;


// define standard relocation ops as `types`
using K_REL_NONE     = KAS_STRING("K_REL_NONE");
using K_REL_ADD      = KAS_STRING("K_REL_ADD");
using K_REL_SUB      = KAS_STRING("K_REL_SUB");
using K_REL_GOT      = KAS_STRING("K_REL_GOT");
using K_REL_PLT      = KAS_STRING("K_REL_PLT");
using K_REL_COPY     = KAS_STRING("K_REL_COPY");
using K_REL_GLOB_DAT = KAS_STRING("K_REL_GLOB_DAT");
using K_REL_JMP_SLOT = KAS_STRING("K_REL_JMP_SLOT");

struct reloc_op_fns
{
    using value_t  = int64_t;
    
    struct update_t : std::pair<value_t, bool>
    {
        constexpr update_t(value_t data = {}, bool is_updated = true)
                    : std::pair<value_t, bool>(data, is_updated) {}
    };

    constexpr reloc_op_fns() {}

    // default: read/write return data/value un-modified
    virtual value_t  read  (value_t data)                 const
        { return data;  }
    virtual const char *write (value_t& data, value_t value)  const
        { data = value; return {}; }
    
    // `update_t` is std::pair. `bool` attribute true if update defined
    // default: return addend unmodified
    virtual update_t update(value_t data, value_t addend) const
        { return { addend, false }; };

    // assembler should emit reloc without relocation
    // XXX should be marked for deletion?? Assembler only??
    virtual bool emit_bare() const { return false; }
};

struct reloc_action
{
    using index_t = uint8_t;

    // constructors
    reloc_action() = default;

    // canonical ctor
    constexpr reloc_action(index_t n) : index(n) {}

    // name ctor (not constexpr)
    reloc_action(const char *name)
        : reloc_action(lookup(name)) 
        {}

    // declare defined action data
    static const reloc_op_fns * const *ops;
    static const char         * const *names;
    static const index_t               MAX_ACTIONS;

    // access action info
    auto& get()  const { return *ops[index]; }
    auto  name() const { return names[index]; }

    // used by `kbfd_reloc` to generate key value
    constexpr auto key() const { return index; }

    // method to map `name` to indexes
    // NB: return type allows out-of-range values
    static index_t lookup(const char *);

    // this method is for `kbfd_reloc_info` ctor
    // NB: requires all arch includes to be present
    template <typename T>
    static constexpr index_t as_action(T);

private:
    index_t index {};
};
}
#endif
