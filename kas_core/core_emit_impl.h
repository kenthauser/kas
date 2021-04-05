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
void core_emit::assert_width() const
{
    if (!width)
        throw std::logic_error("core_emit: width not set");
}

void core_emit::set_width(std::size_t w)
{
    if (width && width != w)
        throw std::logic_error("core_emit: width previously set to "
                                 + std::to_string(width));
    width = w;
}

void core_emit::set_chan(e_chan_num chan)
{
    e_chan = chan;
    width  = sizeof(expression::e_addr_t);  // default size for non-data channels
}

kbfd::kbfd_target_reloc const* core_emit::get_reloc(kbfd::kbfd_reloc& reloc) const
{
    // if no width specified by reloc, use current width
    reloc.default_width(width);
    return obj_p->get_reloc(reloc);
}

core_reloc& core_emit::add_reloc(core_reloc&& r)
{
    if (reloc_p == std::end(relocs))
        throw std::runtime_error("core_emit: too many relocations for insn");
    
    *reloc_p++ = std::move(r);
    return reloc_p[-1];
}

void core_emit::put_fixed(int64_t value, uint8_t obj_width)
{
    if (!width)             // if not explicitly set
        set_width(obj_width);
    data = value;
    emit_obj_code();
}

void core_emit::set_defaults()
{
    e_chan      = EMIT_DATA;
    data        = {};
    width       = {};
    reloc_p     = relocs.begin();
}

// apply relocs, push fixed data to stream (advancing position), reset defaults
void core_emit::emit_obj_code()
{
    // width must be set to emit
    assert_width();

    // emit pending RELOCs.
    parser::kas_error_t diag;
    for (auto p = relocs.begin(); p != reloc_p; ++p)
        p->emit(*this, diag);

    // if relocs completed w/o error, emit base value
    if (diag)
        (*this)(diag.get());
    else
        stream.put_uint(e_chan, width, data);
    set_defaults();
}

void core_emit::set_segment(core_segment const& segment)
{
    section_p = &segment.section();
    stream.set_section(*section_p);
}

std::size_t core_emit::position() const
{
    return stream.position();
}

core_section const& core_emit::get_section() const
{
    return *section_p;
}


// handle `expr_t`:
//  1) if constant expression, extract value
//  2) otherwise, treat as relocatable
//  3) -> emit object code

void core_emit::operator()(expr_t const& e)
{
    auto fixed_p = e.get_fixed_p();

    if (fixed_p)
        put_fixed(*fixed_p);
    else
    {
        auto loc_p = e.get_loc_p();   
        e.apply_visitor(
            x3::make_lambda_visitor<void>
                ([this, &loc_p](auto const& value)
                    { return (*this)(value, loc_p); }
            ));
    }
}

// handle `diag`: emit error into error stream
void core_emit::operator()(parser::kas_diag_t const& diag, kas_loc const *loc_p)
{
    stream.put_diag(e_chan, width, diag);
    set_defaults();
}

// handle "internal" methods as adding relocation, then emitting
void core_emit::operator()(core_addr_t const& addr, kas_loc const *loc_p)
{
    // add relocation & emit
    add_reloc()(addr, loc_p);
    emit_obj_code();    
}


void core_emit::operator()(core_symbol_t const& sym, kas_loc const *loc_p)
{
    // add relocation & emit
    add_reloc()(sym, loc_p);
    emit_obj_code();    
}

void core_emit::operator()(core_expr_t const& expr, kas_loc const *loc_p)
{
    // add relocation & emit
    add_reloc()(expr, loc_p);
    emit_obj_code();    
}

template <typename T>
void core_emit::operator()(T const& e, kas_loc const *loc_p)
{
    std::cout << "core_emit: unsupported expression: " << expr_t(e) << std::endl;
}

void core_emit::put_section_reloc(core_reloc& r, core_section const& section)
{
    auto tgt_reloc_p = get_reloc(r.reloc);
    if (!tgt_reloc_p)
    {
        std::cout << "no target relocation for action = " << r.reloc.action.name();
        std::cout << " bits = " << +r.reloc.bits << std::endl;
        // ** put diag **
        return;
    }

    stream.put_section_reloc(e_chan, *tgt_reloc_p, r.offset, section, r.addend);
}

void core_emit::put_symbol_reloc (core_reloc& r, core_symbol_t const& symbol)
{
    auto tgt_reloc_p = get_reloc(r.reloc);
    if (!tgt_reloc_p)
    {
        std::cout << "no target relocation for action = " << r.reloc.action.name();
        std::cout << " bits = " << +r.reloc.bits << std::endl;
        // ** put diag **
        return;
    }

    stream.put_symbol_reloc(e_chan, *tgt_reloc_p, r.offset, symbol, r.addend);
}

}   // namespace kas_core
#endif
