#ifndef KAS_CORE_CORE_RELOC_IMPL_H
#define KAS_CORE_CORE_RELOC_IMPL_H

#include "core_reloc.h"
#include "core_symbol.h"

#include "kbfd/kbfd_target_reloc.h"     // for print

namespace kas::core
{
// complete construction of `reloc`
void core_reloc::operator()(expr_t const& e, uint8_t flags)
{
    r_flags |= flags;               // eg. emit `bare` reloc

    // use an `if tree` used to initialize relocation
    // don't need `apply_visitor` as most types don't relocate
    if (auto p = e.get_fixed_p())
    {
        // just update "fixed" value
        addend += *p;
    }
    else if (auto p = e.template get_p<core_symbol_t>())
        (*this)(*p);
    else if (auto p = e.template get_p<core_expr_t>())
        (*this)(*p);
    else if (auto p = e.template get_p<core_addr_t>())
        (*this)(*p);
    else if (auto p = e.template get_p<parser::kas_diag_t>())
        diag_p = p;
    else
    {
        // XXX generate `kas_diag_t`
        std::cout << "core_reloc::operator(): unsupported expression" << std::endl;
    }

    if (diag_p)
        (*this)(*diag_p);
}

// symbols can vary. Filter out `EQU` symbols.
void core_reloc::operator()(core_symbol_t const& value)
{
#ifdef TRACE_CORE_RELOC
    std::cout << "core_reloc::()(core_symbol_t const&): " << value << std::endl;
#endif
    // if `EQU`, interpret value
    if (auto p = value.value_p())
        (*this)(*p);

    // otherwise, resolve as symbol
    else
        sym_p = &value;
}

void core_reloc::operator()(core_addr_t const& value)
{
#ifdef TRACE_CORE_RELOC
    std::cout << "core_reloc::()(core_addr_t const&): " << value;
    std::cout << " section = " << value.section();
    std::cout << " offset  = " << value.offset()();
    std::cout << std::endl;
#endif
    addend   +=  value.offset()();
    section_p = &value.section();
}

void core_reloc::operator()(parser::kas_diag_t const& value)
{
#ifdef TRACE_CORE_RELOC
    // XXX cast should not be required...
    std::cout << "core_reloc::()(kas_diag_t&): " << expr_t(value) << std::endl;
#endif

    diag_p = &value;
}

void core_reloc::operator()(core_expr_t const& value)
{
#ifdef TRACE_CORE_RELOC
    std::cout << "core_reloc::()(core_expr_t const&): " << value << std::endl;
#endif

    core_expr_p = &value;
}

#if 0
// XXX deal with PC_REL, SB_REL, etc
kbfd::kbfd_target_reloc const *core_reloc::
        get_tgt_reloc(core_emit& base)
{
    // XXX deal with PC_REL, SB_REL, etc

    auto p = base.get_reloc(reloc);

    if (!p && loc)
    {
        std::ostringstream msg;
        msg << "no target relocation for \"" << reloc << "\"";
        std::cout << "core_reloc::get_tgt_reloc: " << msg.str() << std::endl;
        auto& diag = e_diag_t::error(msg.str(), loc);
        base.error_p = &diag;
    }
    return p;
}
#endif

void core_reloc::emit(core_emit& base, parser::kas_error_t& diag)
{
#ifdef TRACE_CORE_RELOC
    std::cout << "core_reloc::emit: reloc = " << reloc << std::dec;
    std::cout << ", addend = " << addend << std::hex << ", data = " << base.data;
    if (sym_p)
        std::cout << " sym = " << *sym_p;
    else if (core_expr_p)
        std::cout << " expr = " << *core_expr_p;
    else if (section_p)
        std::cout << " section = " << *section_p;
    else if (diag_p)
        std::cout << " *diag*";
    else 
        std::cout << " *bare reloc*";
    std::cout << std::endl;
#endif

    // XXX may duplicate logic in: core_emit::put_reloc(..., symbol&)
    // need to select proper `non-global` symbol base
    // consult with backend via `base.reloc_select_base_sym`
    if (sym_p && sym_p->binding() != STB_GLOBAL)
    {
        // if `sym_p` holds address, try select symbol
        if (auto p = sym_p->addr_p())
        {
            sym_p = base.reloc_select_base_sym(*sym_p);
            if (!sym_p)
                (*this)(*p);
        }
    }
    
    // deal with PC_REL, SB_REL, etc...
    // XXX shouldn't this be in `base`
    //auto tgt_reloc_p = get_tgt_reloc(base);
    // XXX deleted processing of `diag&`

    // emit relocations to backend
    if (section_p)
        base.put_reloc(*this, *section_p);
    else if (sym_p)
        base.put_reloc(*this, *sym_p);
    else if (core_expr_p)
        core_expr_p->emit(base, *this, diag);
    else
        base.put_reloc(*this);      // bare reloc
#if 0
    // XXX let KBFD apply `addend` to base
    // if "addend", apply as relocation to base
    if (addend)
    {
        auto err = apply_reloc(base, diag);
        if (err && loc)
        {
            auto& diag = e_diag_t::error(err, loc);
            base.error_p = &diag;
            std::cout << "core_reloc::emit(): Err = " << err << std::endl;
        }
        else if (err)
            std::cout << "core_reloc::emit(): Err = \"" << err << "\" (no loc)" << std::endl;
    }
#endif
}
#if 0
void core_reloc::put_reloc(core_emit& base, parser::kas_error_t& diag 
                                , core_section const& section)
{
#ifdef TRACE_CORE_RELOC
    std::cout << "put_reloc::put_reloc (section): reloc = " << reloc;
    std::cout << ", addend = " << addend << ", data = " << base.data;
    std::cout << std::endl;
#endif
#if 0
    // absorb section_p if PC_REL && matches
    // NB: could be done in `add`, but `core_reloc` doesn't know `base`
    if (section_p == &base.get_section())
        if (reloc.has(RFLAGS_PC_REL))
        {
            section_p    = {};
            addend      -= base.position();
            //reloc.clear(RFLAGS_PC_REL);
            return;
        }
#endif
    base.put_reloc(*this, section);
}

void core_reloc::put_reloc(core_emit& base, parser::kas_error_t& diag 
                                , core_symbol_t const& sym)
{
    base.put_reloc(*this, sym);
}

void core_reloc::put_reloc(core_emit& base, parser::kas_error_t& diag)
{
    base.put_reloc(*this);
}

// Apply `reloc_fn`: deal with offsets & width deltas
const char *core_reloc::apply_reloc(core_emit& base, parser::kas_error_t& diag)
{
#ifdef TRACE_CORE_RELOC
    std::cout << "core_reloc::apply_reloc: reloc = " << reloc;
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
#endif
    // XXX need `emit_stream_base::emit_value_t` to be picked up from `kbfd`
    static_assert(std::is_same_v<typename core_emit::emit_value_t
                               , typename kbfd::reloc_op_fns::value_t>
                , "emit_value_t value is incorrect");

    // apply reloc(addend) to data
    auto& ops = reloc.get();

    auto value = ops.update(ops.read(base.data), addend).first;
    auto err   = ops.write(base.data, value);

#ifdef TRACE_CORE_RELOC
    std::cout << "put_reloc::apply_reloc: result = " << base.data;
    if (err)
        std::cout << ", *ERROR* = " << err;
    std::cout << std::endl;
#endif
    return err;
}
// static method
// return true iff `relocs` emited OK
// XXX how is diag emitted???
bool core_reloc::done(core_emit& base) { return true; }
#endif


}
#endif
