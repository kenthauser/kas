#ifndef KAS_CORE_CORE_RELOC_IMPL_H
#define KAS_CORE_CORE_RELOC_IMPL_H

#include "core_reloc.h"
#include "core_symbol.h"

#include "kbfd/kbfd_target_reloc.h"     // for print

namespace kas::core
{
// complete construction of `reloc`
void core_reloc::operator()(expr_t const& e)
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
        std::cout << "core_reloc::operator(): unsupported type" << std::endl;
    }
}

// symbols can vary. Sort by type of symbol
void core_reloc::operator()(core_symbol_t const& value, kas_loc const *loc_p)
{
    // save location if specified
    if (loc_p)
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

void core_reloc::operator()(core_addr_t const& value, kas_loc const *loc_p)
{
    // save location if specified
    if (loc_p)
        this->loc_p = loc_p;

    addend   +=  value.offset()();
    section_p = &value.section();
}

void core_reloc::operator()(parser::kas_diag_t const& value, kas_loc const *loc_p)
{
    // save location if specified
    if (loc_p)
        this->loc_p = loc_p;

    diag_p      = &value;
}

void core_reloc::operator()(core_expr_t const& value, kas_loc const *loc_p)
{
    // save location if specified
    if (loc_p)
        this->loc_p = loc_p;

    core_expr_p = &value;
}

void core_reloc::emit(core_emit& base, parser::kas_error_t& diag)
{
    std::cout << "put_reloc::emit: reloc = " << reloc;
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

    // if symbol, reinterpret before emitting
    if (sym_p)
    {
        auto& sym = *sym_p;
        sym_p = {};
        (*this)(sym);
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

void core_reloc::put_reloc(core_emit& base, parser::kas_error_t& diag 
                                , core_section const& section)
{
    std::cout << "put_reloc::put_reloc (section): reloc = " << reloc;
    std::cout << ", addend = " << addend << ", data = " << base.data;
    std::cout << std::endl;

    // absorb section_p if PC_REL && matches
    // NB: could be done in `add`, but `core_reloc` doesn't know `base`
    if (section_p == &base.get_section())
        if (reloc.has(RFLAGS_PC_REL))
        {
            section_p    = {};
            addend      -= base.position();
            reloc.clear(RFLAGS_PC_REL);
            return;
        }
    base.put_section_reloc(*this, section);
}

void core_reloc::put_reloc(core_emit& base, parser::kas_error_t& diag 
                                , core_symbol_t const& sym)
{
    base.put_symbol_reloc(*this, sym);
}

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

    // apply reloc(addend) to data
    auto& ops = reloc.get();

    auto value = ops.update(ops.read(base.data), addend).first;
    base.data  = ops.write(base.data, value);

    std::cout << "put_reloc::apply_reloc: result = " << base.data << std::endl;
}

// static method
// return true iff `relocs` emited OK
bool core_reloc::done(core_emit& base) { return true; }


}
#endif
