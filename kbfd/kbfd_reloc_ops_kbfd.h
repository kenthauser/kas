#ifndef KAS_ELF_RELOC_OPS_KBFD_H
#define KAS_ELF_RELOC_OPS_KBFD_H


#include "kbfd_reloc_ops.h"

namespace kbfd
{


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
    const char *write(value_t& data, value_t value) const override
    {
        auto n = data &~ (MASK << SHIFT);
        data   = n | (value & MASK) << SHIFT;
        return {};
    }
};

// need `type` for generic relocation ops. Pick `K_REL_NONE`
using reloc_ops_defn_tags = meta::push_front<target_tags, K_REL_NONE>;

template <> struct reloc_ops_v<K_REL_NONE> : meta::list<
        KBFD_ACTION<K_REL_NONE  , reloc_op_fns>
      , KBFD_ACTION<K_REL_ADD   , k_rel_add_t>
      , KBFD_ACTION<K_REL_SUB   , k_rel_sub_t>

      // XXX to be defined
      , KBFD_ACTION<K_REL_GOT   , reloc_op_fns>
      , KBFD_ACTION<K_REL_PLT   , reloc_op_fns>
      , KBFD_ACTION<K_REL_COPY  , reloc_op_fns>
      , KBFD_ACTION<K_REL_GLOB_DAT, reloc_op_fns>
      , KBFD_ACTION<K_REL_JMP_SLOT, reloc_op_fns>
      > {};


#if 0
// Apply `reloc_fn`: deal with offsets & width deltas
void core_reloc::apply_reloc(core_emit& base, parser::kas_error_t& diag)
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
