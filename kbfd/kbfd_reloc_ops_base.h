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

#if 0
// Apply `reloc_fn`: deal with offsets & width deltas
void core_reloc::apply_reloc(emit_base& base, parser::kas_error_t& diag)
{
    std::cout << "put_reloc::apply_reloc: reloc = " << reloc;
    std::cout << ", addend = " << addend << ", data = " << base.data;
    if (sym_p)
        std::cout << " sym = " << *sym_p;
    else if (core_expr_p)
        std::cout << " expr = " << *core_expr_p;
    else if (section_p)
        std::cout << " section = " << *section_p;
    else if (diag_p)
        std::cout << " *diag*";
    std::cout << std::endl;

#if 1
    // apply reloc(addend) to data
    auto& ops = reloc.get();

    auto value = ops.update(ops.read(base.data), addend).first;
    base.data  = ops.write(base.data, value);

    std::cout << "put_reloc::apply_reloc: result = " << base.data << std::endl;
#else
    auto read_subfield = [&](int64_t data) -> std::tuple<int64_t, uint64_t, uint8_t>
        {
            uint64_t mask  {};
            uint8_t  shift {};
            auto     width = reloc.bits / 8;
#if 0
            std::cout << "apply_reloc: width = " << +width;
            std::cout << ", offset = " << +offset;
            std::cout << ", base_width = " << +base.width;
            std::cout << std::endl;

            std::cout << "location: " << *base.section_p;
            std::cout << "+" << std::hex << base.stream.position() << std::endl;
#endif     
            if (base.width < (width + offset))
                throw std::logic_error { "apply_reloc: invalid width/offset" };

            // normal case: matching widths
            if (base.width == width)
                return { data, 0, 0 };

            // constexpr to calculate mask
            mask  = (width == 4) ? 0xffff'ffff : (width == 2) ? 0xffff : (width == 1) ? 0xff : ~0; 
            shift = (base.width - offset - 1) * 8;
           
            // apply offset & delta width operations
            mask <<= shift;
            data = (data & mask) >> shift;

            // sign-extend data
            switch(width)
            {
                case 1: data = static_cast<int8_t >(data); break;
                case 2: data = static_cast<int16_t>(data); break;
                case 4: data = static_cast<int32_t>(data); break;
                default: break;
            }
            return { data, mask, shift };
        };

    auto write_subfield = [](auto base, auto value, auto mask, auto shift)
                -> decltype(base)
        {
            // normal case. matching widths
            if (!mask) return value;

            // remove result bits from original data & insert shifted value
            base &=~ mask;
            base |=  mask & (value << shift);
            return base;
        };

    // read data as subfield
    auto [data, mask, shift] = read_subfield(base.data);

    // working copy of data is base `addend`
    auto value = data;
#if 0 
    // retrieve reloc methods
    auto& ops = base.elf_reloc_p->get_ops(reloc);
    
    // if `read_fn` extract addend from `data`
    //if (ops.read_fn)
        value = ops.read(value);

    // apply `update_fn` on value
    value = ops.update(value, addend).first;
    addend = {};        // consumed

    // if `write_fn` update extracted data
    //if (ops.write_fn)
        value = ops.write(data, value);

    // make sure fixed value is in range
    expression::expr_fits fits;
    if (fits.disp_sz(base.width, value, 0) != fits.yes)
    {
        auto msg = "base relocation out-of-range for object format";
#if 0
        parser::kas_loc *loc_p = {};
        if (sym_p)
            loc_p = &sym_p->loc();
        else if (section_p)
            loc_p = &section_p->loc();
        else if (core_expr_p)
            loc_p = &core_expr_p->loc();
#endif
        if (!loc_p)
            std::cout << "core_reloc::apply_reloc: no `loc` for error" << std::endl;
        else
            diag = e_diag_t::error(msg, *loc_p).ref();
    }
        
    // insert new `data` as subfield
    base.data = write_subfield(base.data, value, mask, shift);
#endif
#endif
}
#endif
}

#endif
