#ifndef KAS_CORE_CORE_EMIT_IMPL_H
#define KAS_CORE_CORE_EMIT_IMPL_H

#include "expr/expr.h"
#include "emit_reloc.h"
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
        throw std::runtime_error("emit_base: width not set");
}

void emit_base::set_width(std::size_t w)
{
    if (width && width != w)
        throw std::runtime_error("emit_base: width previously set to "
                                 + std::to_string(width));
    width = w;
}

void emit_base::set_chan(e_chan_num chan)
{
    e_chan = chan;
}

void emit_base::put_fixed(std::size_t obj_width, uint64_t obj_data)
{
    if (!width)             // if not explicitly set
        set_width(obj_width);
    data += obj_data;
    emit_fixed();
}

void emit_base::set_defaults()
{
    data      = {};
    width     = {};
    e_chan    = EMIT_DATA;
}

// push fixed data to stream, advancing position, reset defaults
void emit_base::emit_fixed()
{
    // `kas_diag` can emit "special" data. clears width to flag.
    if (width)
        stream.put_uint(e_chan, width, data);
    set_defaults();
}


// extract wrapped type from expression variant
void emit_base::operator()(expr_t const& e)
{
    e.apply_visitor(*this);
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

emit_base& emit_base::operator<<(expr_t const& expr)
{
    assert_width();
    (*this)(expr);      // evaluate & apply
    emit_fixed();       // emit fixed data, advance dot, reset()
    return *this;       // return
}

#if 0
emit_base& emit_base::operator<<(std::string const& str)
{
    (*this)(str);       // apply
    emit_fixed();       // emit fixed data, advance dot, reset()
    return *this;       // return
}
#endif

void emit_base::operator<<(parser::kas_diag const& diag)
{
    // diag can have zero width
    (*this)(diag);      // emit
    set_defaults();     // prepare for next. Don't chain
}

void emit_base::operator()(std::string const& str)
{
    stream.put_str(e_chan, width, str);
}


template <typename T>
std::enable_if_t<std::is_integral<T>::value>
emit_base::operator()(T t)
{
    assert_width();
    data += t;
}


void emit_base::operator()(core_addr const& addr)
{
    assert_width();
    
    // add offset to data & emit
    data += addr.offset()();
    
    auto& r = emit_relocs[R_DIRECT];
    stream.put_section_reloc(e_chan, r(width), addr.section(), data);
}


void emit_base::operator()(core_symbol const& sym)
{
    assert_width();
    
    // if symbol has defined address or value, use those
    if (auto p = sym.addr_p())
        return (*this)(*p);
    if (auto p = sym.value_p())
        return (*this)(*p);

    // undefined/external symbol. emit symbol reloc
    auto& r = emit_relocs[R_DIRECT];
    stream.put_symbol_reloc(e_chan, r(width), sym, data);
}

void emit_base::operator()(core_expr const& expr)
{
    if (!width)
        width = sizeof_data_t;
    expr.emit(*this);
}

void emit_base::operator()(parser::kas_diag const& diag)
{
    // emit error message into error stream
    stream.put_diag(e_chan, width, diag);
    width = 0;      // suppress duplicate data write
}

 // catch-all for not-yet-implemented types
template <typename T, typename>
void emit_base::operator()(T const& t)
{
    std::ostringstream str;
    str << width << "[";
    print_object(str, t);
    str << "]";
    stream.put_str(e_chan, width, str.str());
}

}

#endif
