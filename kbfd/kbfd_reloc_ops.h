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
using K_REL_ADD      = KAS_STRING("K_REL_ADD");     // ignore overflow
using K_REL_SUB      = KAS_STRING("K_REL_SUB");
using K_REL_SADD     = KAS_STRING("K_REL_SADD");    // error on overflow
using K_REL_SSUB     = KAS_STRING("K_REL_SSUB");
using K_REL_GOT      = KAS_STRING("K_REL_GOT");
using K_REL_PLT      = KAS_STRING("K_REL_PLT");
using K_REL_COPY     = KAS_STRING("K_REL_COPY");
using K_REL_GLOB_DAT = KAS_STRING("K_REL_GLOB_DAT");
using K_REL_JMP_SLOT = KAS_STRING("K_REL_JMP_SLOT");

// forward declare types
struct reloc_host_fns;

struct reloc_op_fns
{
    // work around g++ virtual base bug
    using vt_base_t = reloc_op_fns;  // for `init_for_list`
    
    using value_t   = int64_t;       // integral value
  
    // include loop: `kfbd_reloc` & `reloc_op_fns` both define `flags_t`
    // NB: this definition should match `kbfd_reloc` defn
    using flags_t  = uint16_t;      // NB: static assert in `eval` impl

    // for passing arguments to/from reloc ops
    // NB: void * args are interpreted locally (by `kbfd` or by `client`) 
    struct arg_t
    {
        arg_t(value_t v = {}, void *seg_p = {}, void *sym_p = {})
            : value(v), seg_p(seg_p), sym_p(sym_p) {}

        value_t value;
        void   *seg_p;
        void   *sym_p;

        // return true if `arg_t` is zero
        bool empty() const
            { return !(value || seg_p || sym_p); }

        // return true if `arg_t` is non-zero
        operator bool() const
            { return !empty(); }
    };

    // need constexpr ctor for `init_from_list` metafunction
    // NB: no virtual dtor because not allowed for constexpr objects
    constexpr reloc_op_fns() {}

    // read from target as integral value (used as ACCUM)
    virtual value_t extract (value_t data) const;
    
    // insert `value` into `data`
    virtual const char *insert(value_t& data, value_t value) const;
    
    // decode value read from target: default is pass-thru
    virtual value_t decode (value_t data) const      { return data; }

    // encode value to write to target: default is pass-thru
    virtual const char *encode (value_t& data) const { return {}; }

    // evaluate relocation. ie get value to pass to `update`
    // eg: consume PC_REL or SB_REL if appropriate
    // XXX 
    virtual const char *eval (flags_t flags
                            , value_t&  value) const;

    // perform reloc operation: update `accum` with `value` per `op`
    // NB: passed `kbfd_reloc::flags` to inform conversion
    virtual const char *update(flags_t        flags
                             , value_t&       accum
                             , value_t const& value) const;

    // should emit reloc without relocation symbol
    // example: arm32: R_ARM_V4BX
    virtual bool emit_bare() const { return false; }
};

struct reloc_host_fns
{
    using value_t = reloc_op_fns::value_t;

    // not used by `init_from_list`, so don't need constexpr ctor
    reloc_host_fns() = default;

};

// map reloc NAME to ACTION (for `init_from_list`, assembler, et al)
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
