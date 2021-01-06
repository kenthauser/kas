#ifndef KAS_CORE_CORE_RELOC_IMPL_H
#define KAS_CORE_CORE_RELOC_IMPL_H

#include "core_reloc.h"

namespace kas::core
{
// complete construction of `reloc`
void deferred_reloc_t::operator()(expr_t const& e)
{
    if (!loc_p)
        loc_p = e.get_loc_p();      // if `tagged` store location

    // `if tree` used to initialize relocation
    // don't need `apply_visitor` as most types don't relocate
    if (auto p = e.get_fixed_p())
    {
        // just update "fixed" value
        addend += *p;
    }
    else if (auto p = e.template get_p<core_symbol_t>())
        (*this)(*p, loc_p);
    else if (auto p = e.template get_p<core_expr_t>())
        (*this)(*p, loc_p);
    else if (auto p = e.template get_p<core_addr_t>())
        (*this)(*p, loc_p);
    else if (auto p = e.template get_p<parser::kas_diag_t>())
        (*this)(*p, loc_p);
    else
    {
        std::cout << "deferred_reloc_t::operator(): unsupported type" << std::endl;
    }
}

// symbols can `vary`. Sort by type of symbol
void deferred_reloc_t::operator()(core_symbol_t const& value, kas_loc const *loc_p)
{
    this->loc_p = loc_p;
    // see if resolved symbol
    if (auto p = value.addr_p())
        (*this)(*p);

    // if `EQU`, interpret value
    else if (auto p = value.value_p())
        (*this)(*p);

    // otherwise, unresolved symbol
    else
        sym_p = &value;
}

void deferred_reloc_t::operator()(core_addr_t const& value, kas_loc const *loc_p)
{
    this->loc_p = loc_p;
    addend   +=  value.offset()();
    section_p = &value.section();
}

void deferred_reloc_t::operator()(parser::kas_diag_t const& value, kas_loc const *loc_p)
{
    this->loc_p = loc_p;
    diag_p      = &value;
}

void deferred_reloc_t::operator()(core_expr_t const& value, kas_loc const *loc_p)
{
    this->loc_p = loc_p;
    
    if (auto p = value.get_fixed_p())
        addend += *p;
    else
        core_expr_p = &value;
}

void deferred_reloc_t::emit(emit_base& base, parser::kas_error_t& diag)
{
    // if symbol, reinterpret before emitting
    if (sym_p)
    {
        auto& sym = *sym_p;
        sym_p = {};
        (*this)(sym, loc_p);
    }

    // if no width specified by reloc, use current width
    if (reloc.bits == 0)
        reloc.bits = base.width * 8;
    
    // absorb section_p if PC_REL && matches
    // NB: could be done in `add`, but `deferred_reloc_t` doesn't know `base`
    if (section_p == &base.get_section())
        if (reloc.flags & kbfd::kas_reloc::RFLAGS_PC_REL)
        {
            section_p    = {};
            addend      -= base.position();
            reloc.flags &=~ kbfd::kas_reloc::RFLAGS_PC_REL;
        }
   
    // emit relocations to backend
    if (section_p)
        put_reloc(base, diag, *section_p);
    else if (sym_p)
        put_reloc(base, diag, *sym_p);
    else if (core_expr_p)
        core_expr_p->emit(base, *this, diag);
    else if (diag_p)
        diag = *diag_p;

    // if "addend", apply as relocation to base
    if (addend)
        apply_reloc(base, diag);
}

void deferred_reloc_t::put_reloc(emit_base& base, parser::kas_error_t& diag 
                                , core_section const& section)
{
    // get pointer to machine-specific info matching `reloc`
    kbfd::kas_reloc_info const *info_p {};
    auto msg = base.elf_reloc_p->get_info(reloc, &info_p);
    if (msg)
    {
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
            std::cout << "deferred_reloc_t::put_reloc: no `loc` for error" << std::endl;
        else
            diag = e_diag_t::error(msg, *loc_p).ref();
    }
    else
        base.put_section_reloc(*this, info_p, section, addend);
}

void deferred_reloc_t::put_reloc(emit_base& base, parser::kas_error_t& diag 
                                , core_symbol_t const& sym)
{
    // get pointer to machine-specific info matching `reloc`
    kbfd::kas_reloc_info const *info_p {};
    auto msg = base.elf_reloc_p->get_info(reloc, &info_p);
    if (msg)
    {
        if (!loc_p)
            std::cout << "deferred_reloc_t::put_reloc: no `loc` for error" << std::endl;
        else
            diag = parser::kas_diag_t::error(msg, *loc_p).ref();
    }
    else
        base.put_symbol_reloc(*this, info_p, sym, addend);
}

// Apply `reloc_fn`: deal with offsets & width deltas
void deferred_reloc_t::apply_reloc(emit_base& base, parser::kas_error_t& diag)
{
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
            std::cout << "deferred_reloc_t::apply_reloc: no `loc` for error" << std::endl;
        else
            diag = e_diag_t::error(msg, *loc_p).ref();
    }
        
    // insert new `data` as subfield
    base.data = write_subfield(base.data, value, mask, shift);
};

// static method
// return true iff `relocs` emited OK
bool deferred_reloc_t::done(emit_base& base) { return true; }


}

#endif
