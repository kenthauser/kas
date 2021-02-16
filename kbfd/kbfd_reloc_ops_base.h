#ifndef KAS_ELF_RELOC_OPS_BASE_H
#define KAS_ELF_RELOC_OPS_BASE_H

namespace kbfd
{


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
    virtual value_t  write (value_t data, value_t value)  const
        { return value; }
    // `update_t` is std::pair. `bool` attribute true if update defined
    virtual update_t update(value_t data, value_t addend) const
        { return { {}, false }; };
};

struct k_rel_add_t : virtual reloc_op_fns
{
    update_t update(value_t data, value_t addend) const override 
    { 
        return data + addend;
    }  
};

struct k_rel_sub_t : virtual reloc_op_fns
{
    update_t update(value_t data, value_t addend) const override
    { 
        return data - addend;
    } 
};


template <int BITS, int SHIFT>
struct reloc_op_subfield : virtual reloc_op_fns
{
    using bits   = meta::int_<BITS>;
    using shift  = meta::int_<SHIFT>;

    static constexpr auto MASK = (1 << BITS) - 1;
    
    value_t read(value_t data) const override
    {
        return (data >> SHIFT) & MASK; 
    }
    value_t write(value_t data, value_t value) const override
    {
        data &=~ (MASK << SHIFT);
        return data | (value & MASK) << SHIFT;
    }
};

// kas_defn declaration for `reloc_ops` definitions
template <typename = void> struct reloc_ops_v : meta::list<> {};

// format each "defn" as "list" as required for `init_from_list`
template <typename NAME, typename OP, int...Ts>
using KBFD_ACTION = meta::list<NAME, OP>;//, NAME, meta::int_<Ts>...>;


struct reloc_action
{
    using index_t = uint8_t;

    // constructors
    reloc_action() = default;

    // cononical ctor
    constexpr reloc_action(index_t n) : index(n) {}

    // name ctor
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