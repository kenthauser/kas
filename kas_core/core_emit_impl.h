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
    // XXX short circuit on `reloc_p`
    if (reloc_p)
        emit_relocs();
#if 0 
    if (error_p)     
        std::cout << "core_emit::emit_obj_code: error_p set (1)" << std::endl;
#endif
    // if relocs completed w/o error, emit base value
#if 1
    if (error_p)
    {
        //std::cout << "core_emit::emit_obj_code: error_p set (2)" << std::endl;
        (*this)(*error_p);
    }
#else
    if (diag)
        (*this)(diag.get());
#endif
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
        e.apply_visitor(
            x3::make_lambda_visitor<void>
                ([this](auto const& value)
                    { return (*this)(value); }
            ));
    }
}

// handle `diag`: emit error into error stream
void core_emit::operator()(parser::kas_diag_t const& diag)
{
    stream.put_diag(e_chan, width, diag);
    set_defaults();
}

// handle "internal" methods as adding relocation, then emitting
void core_emit::operator()(core_addr_t const& addr)
{
    // add relocation & emit
    add_reloc()(addr);
    emit_obj_code();    
}


void core_emit::operator()(core_symbol_t const& sym)
{
    // add relocation & emit
    add_reloc()(sym);
    emit_obj_code();    
}

void core_emit::operator()(core_expr_t const& expr)
{
    // if `fixed` emit as fixed
    //std::cout << "core_emit::operator(core_expr_t const&)";
    if (auto p = expr.get_fixed_p())
    {  std::cout << std::endl;
        (*this)(*p);
    }
    else
    {
        //std::cout << ": requires reloc" << std::endl;
        // add relocation & emit
        add_reloc()(expr);
        emit_obj_code();    
    }
}

template <typename T>
void core_emit::operator()(T const& e)
{
    std::cout << "core_emit: unsupported expression: " << expr_t(e) << std::endl;
}

}   // namespace kas_core
#endif
