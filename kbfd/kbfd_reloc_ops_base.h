#ifndef KAS_ELF_RELOC_OPS_BASE_H
#define KAS_ELF_RELOC_OPS_BASE_H

namespace kbfd
{


struct reloc_op_fns
{
    using value_t  = int64_t;
    

    struct update_t : std::pair<value_t, bool>
    {
        constexpr update_t(value_t data = {}, bool defined = true)
                    : std::pair<value_t, bool>(data, defined) {}
    };

    constexpr reloc_op_fns(const char *name = "K_REL_UNKN") 
                    : name(name) {}

    // default: read/write return data/value un-modified
    virtual value_t  read  (value_t data)                 const { return data;  }
    virtual value_t  write (value_t data, value_t value)  const { return value; }

    // `update_t` is std::pair. `bool` attribute true if update defined
    virtual update_t update(value_t data, value_t addend) const { return {}; };
   
    const char *name;
};


template <int BITS, int SHIFT>
struct reloc_op_subfield : virtual reloc_op_fns
{
    using base_t = reloc_op_fns;
    using bits   = meta::int_<BITS>;
    using shift  = meta::int_<SHIFT>;

    using base_t::base_t;

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
template <typename = void> struct X_reloc_ops_v : meta::list<> {};

// format each "defn" as "list" as required for `init_from_list`
template <typename NAME, typename OP, int...Ts>
using KBFD_ACTION = meta::list<NAME, OP, NAME, meta::int_<Ts>...>;


}

#endif
