#ifndef KAS_CORE_CORE_RELOC_IMPL_H
#define KAS_CORE_CORE_RELOC_IMPL_H

#include "core_reloc.h"
#include "core_symbol.h"

#include "kbfd/kbfd_target_reloc.h"     // for print

namespace kas::core
{
// complete construction of `reloc`
core_reloc& core_reloc::operator()(expr_t const& e, uint8_t flags)
{
    r_flags |= flags;               // eg. emit `bare` reloc

    // use an `if tree` used to initialize relocation
    // don't need `apply_visitor` as most types don't relocate
    if (auto p = e.get_fixed_p())
        value = *p;
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
    return *this;
}

// symbols can vary. Filter out `EQU` symbols.
core_reloc& core_reloc::operator()(core_symbol_t const& value)
{
#ifdef TRACE_CORE_RELOC
    std::cout << "core_reloc::()(core_symbol_t const&): " << value << std::endl;
#endif
    // if `EQU`, interpret value
    if (auto p = value.value_p())
        (*this)(*p);
    
    // if KBFD doesn't know symbol, it must be local address
    // NB: generally, only GLOBAL symbols have `KBFD` symbol number
    else if (!value.sym_num())
        (*this)(*value.addr_p());

    // otherwise, resolve as symbol
    else
        sym_p = &value;
    return *this;
}

core_reloc& core_reloc::operator()(core_addr_t const& value)
{
#ifdef TRACE_CORE_RELOC
    std::cout << "core_reloc::()(core_addr_t const&): " << value;
    std::cout << " section = " << value.section();
    std::cout << " offset  = " << value.offset()();
    std::cout << std::endl;
#endif
    this->value =  value.offset()();
    section_p   = &value.section();
    sym_p       = {};
    return *this;
}

core_reloc& core_reloc::operator()(parser::kas_diag_t const& value)
{
#ifdef TRACE_CORE_RELOC
    // XXX cast should not be required...
    std::cout << "core_reloc::()(kas_diag_t&): " << expr_t(value) << std::endl;
#endif

    diag_p = &value;
    return *this;
}

core_reloc& core_reloc::operator()(core_expr_t const& value)
{
#ifdef TRACE_CORE_RELOC
    std::cout << "core_reloc::()(core_expr_t const&): " << value << std::endl;
#endif

    core_expr_p = &value;
    return *this;
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

void core_reloc::
    gen_diag(core_emit& base, const char *msg, const char *opt) const
{
}

void core_reloc::emit(core_emit& base, int64_t& accum)
{
#ifdef TRACE_CORE_RELOC
    std::cout << "core_reloc::emit: reloc = " << reloc << std::dec;
    std::cout << ", addend = " << addend << ", value = " << value;
    std::cout << std::hex << ", accum = " << accum;
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
#if 0
    // 0. `core_expr` has own `emit` method 
    if (core_expr_p)
        return core_expr_p->emit(base, *this, accum);
#endif

    // 1. resolve symbol to address if appropriate
    if (sym_p)
        if (auto addr_p = sym_p->addr_p())
            if (base.should_resolve(*this, *sym_p))
                (*this)(*addr_p);   // method also clears `sym_p`
    
    // 2. perform PC-REL relocations on addresses if appropriate
    // absorb PCREL if appropriate
    if (base.section_p == section_p)
        if (reloc.has(RFLAGS_PC_REL))
        {
            reloc.clear(RFLAGS_PC_REL);
            section_p = {};
            value    -= base.position();
        }
#if 0
    // XXX may not be resolved by assembler???
    // XXX also test sym_p?? or does not-resolve override??
    if (section_p)
        if (reloc.has(RFLAGS_SB_REL))
        {
            reloc.clear(RFLAGS_SB_REL);
            section_p = {};
        }
#endif

    // 3. update according to reloc. `accum` passed by reference
    //    NB: can add as N-bit, can extract subfield, etc.
    auto& ops = reloc.get();
    auto  msg = ops.update(reloc.flags, accum, value);

#if 0
    if (msg)
        create...diag...
#endif

    // 4. emit reloc to backend. (base holds `accum`, so no need to pass)
    if (sym_p)
        base.put_reloc(*this, *sym_p);      // pass symbol
    else
        base.put_reloc(*this, section_p);   // pass section_p (or nullptr)
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
