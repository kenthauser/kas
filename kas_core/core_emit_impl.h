#ifndef KAS_CORE_CORE_EMIT_IMPL_H
#define KAS_CORE_CORE_EMIT_IMPL_H

#include "expr/expr.h"
#include "emit_stream.h"
#include "core_fragment.h"
#include "core_symbol.h"
#include "core_addr.h"
#include "utility/print_object.h"

#include <iostream>

#include "utility/print_type_name.h"

#include "core_emit.h"

namespace kas::core
{

void emit_base::assert_width() const
{
    if (!width)
        throw std::logic_error("emit_base: width not set");
}

void emit_base::set_width(std::size_t w)
{
    if (width && width != w)
        throw std::logic_error("emit_base: width previously set to "
                                 + std::to_string(width));
    width = w;
}

void emit_base::set_chan(e_chan_num chan)
{
    e_chan = chan;
}

deferred_reloc_t& emit_base::add_reloc(core_reloc r, int64_t addend, uint8_t offset)
{
    if (reloc_p == std::end(relocs))
        throw std::runtime_error("emit_base: too many relocations for insn");

    // set default "RELOC" to `add`
    if (r.reloc == K_REL_NONE)
        r.reloc = K_REL_ADD;
   
    *reloc_p++ = { r, addend, offset };
    return reloc_p[-1];
}

void emit_base::put_fixed(int64_t value, uint8_t obj_width)
{
    if (!width)             // if not explicitly set
        set_width(obj_width);
    data = value;
    emit_obj_code();
}

void emit_base::set_defaults()
{
    e_chan      = EMIT_DATA;
    data        = {};
    width       = {};
    reloc_flags = {};
    reloc_p     = relocs.begin();
}

// apply relocs, push fixed data to stream, advancing position, reset defaults
void emit_base::emit_obj_code()
{
    // width must be set to emit
    assert_width();

    // emit pending RELOCs. 
    for (auto p = relocs.begin(); p != reloc_p; ++p)
        p->emit(*this);

    // if relocs completed w/o error, emit base value
    if (deferred_reloc_t::done(*this))
        stream.put_uint(e_chan, width, data);
    set_defaults();
}

void emit_base::set_segment(core_segment const& segment)
{
    section_p = &segment.section();
    stream.set_section(*section_p);
}

std::size_t emit_base::position() const
{
    return stream.position();
}

core_section const& emit_base::get_section() const
{
    return *section_p;
}


// handle `expr_t`:
//  1) if constant expression, extract value
//  2) otherwise, treat as relocatable
//  3) -> emit object code

void emit_base::operator()(expr_t const& e)
{
    auto fixed_p = e.get_fixed_p();

    if (fixed_p)
        data = *fixed_p;        
    else
        add_reloc()(e);
    emit_obj_code();    
}

// handle `diag`: emit error into error stream
void emit_base::operator()(parser::kas_diag_t const& diag)
{
    stream.put_diag(e_chan, width, diag);
    set_defaults();
}

// handle "internal" methods as relocatable
void emit_base::operator()(core_addr_t const& addr)
{
    // add relocation & emit
    add_reloc()(addr);
    emit_obj_code();    
    //stream.put_section_reloc(e_chan, r(width), addr.section(), data);
}


void emit_base::operator()(core_symbol_t const& sym)
{
    // add relocation & emit
    add_reloc()(sym);
    emit_obj_code();    
    //stream.put_symbol_reloc(e_chan, r(width), sym, data);
}

void emit_base::operator()(core_expr_t const& expr)
{
    // add relocation & emit
    add_reloc()(expr);
    emit_obj_code();    
    //expr.emit(*this);
}

void emit_base::put_section_reloc(deferred_reloc_t const& r, reloc_info_t const *info_p
                     , core_section const& section, int64_t addend)
{
    if (!info_p)
    {
        std::cout << "no info for reloc: code = " << +r.reloc.reloc;
        std::cout << " bits = " << +r.reloc.bits << std::endl;
        return;
    }

    stream.put_section_reloc(e_chan, *info_p, r.width, r.offset, section, addend);
}
void emit_base::put_symbol_reloc (deferred_reloc_t const& r, reloc_info_t const *info_p
                     , core_symbol_t  const& symbol, int64_t addend)
{
    if (!info_p)
    {
        std::cout << "no info for reloc: code = " << +r.reloc.reloc;
        std::cout << " bits = " << +r.reloc.bits << std::endl;
        return;
    }

        // ** put diag **

    stream.put_symbol_reloc(e_chan, *info_p, r.width, r.offset, symbol, addend);
}
    
}

#endif
