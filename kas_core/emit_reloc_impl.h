#ifndef KAS_CORE_EMIT_RELOC_IMPL_H
#define KAS_CORE_EMIT_RELOC_IMPL_H


#include "core_emit.h"

namespace kas::core
{

emit_base& emit_reloc_t::reloc_done() const
{
    if (!reloc_complete)
        throw std::runtime_error("incomplete relocation");
    
    // complete emit & reset defaults.
    base_p->emit_fixed();
    return *base_p;
}
    
// hook reloc into `emit_base` system
emit_reloc_t&& operator<<(emit_base& b, emit_reloc_t&& r)
{
    r.base_p = &b;
    b.set_width(r.width);
    return std::move(r);
}

emit_base& emit_reloc_t::operator<<(expr_t const& e)
    { (*this)(e); return reloc_done(); }

emit_base& emit_reloc_t::operator<<(core_expr const& e)
    { (*this)(e); return reloc_done(); }

emit_base& emit_reloc_t::operator<<(core_addr const& e)
    { (*this)(e); return reloc_done(); }

emit_base& emit_reloc_t::operator<<(core_symbol const& e)
    { (*this)(e); return reloc_done(); }

// intercept symbol & addr relocs until reloc is satisified
void emit_reloc_t::operator()(core_symbol const& e)
{
    if (reloc_complete)
        return (*base_p)(e);
    do_symbol(e);
}

void emit_reloc_t::operator()(core_addr const& e)
{
    if (reloc_complete)
        return (*base_p)(e);
    do_addr(e);
}

// emulate `emit_base` for other types
template <typename T>
void emit_reloc_t::operator()(T const& e)
{
    (*base_p)(e);
}

void emit_reloc_t::operator()(expr_t const& e)
{
    e.apply_visitor(*this);
}

void emit_reloc_t::operator()(core_expr const& e)
{
    e.emit(*this);
}

// only base class is "friends" with `emit_base`
void emit_reloc_t::put_symbol_reloc(uint32_t reloc, core_symbol const& sym) const
{
    base_p->stream.put_symbol_reloc(base_p->e_chan, reloc, sym, base_p->data);
}
void emit_reloc_t::put_section_reloc(uint32_t reloc, core_section const& section) const
{
    base_p->stream.put_section_reloc(base_p->e_chan, reloc, section, base_p->data);
}

//
// PC Relative Relocation support
//

void reloc_pcrel::do_symbol(core_symbol const& sym) 
{
    // if symbol has defined address or value, use those
    if (auto p = sym.addr_p())
        return (*this)(*p);
    if (auto p = sym.value_p())
        return (*this)(*p);

    // undefined/external symbol. emit symbol reloc
    auto& r = emit_relocs[R_PCREL];
    put_symbol_reloc(r(width), sym);
    
    // relocation satisifed
    reloc_complete = true;
}

void reloc_pcrel::do_addr(core_addr const& addr) 
{
    // PC relative offset in current section just adjusts "data"
    // calculate pcrel offset: addr - dot - offset
    (*this)(addr.offset()() - dot.offset()() - offset);
    
    // if reloc not in currect section, also emit a reloc
    if (&addr.section() != &dot.section()) {
        auto& r = emit_relocs[R_PCREL];
        put_section_reloc(r(width), addr.section());
    }

    // relocation satisifed
    reloc_complete = true;
}

}

#endif
