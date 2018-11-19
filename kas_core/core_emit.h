#ifndef KAS_CORE_CORE_EMIT_H
#define KAS_CORE_CORE_EMIT_H

#include "expr/expr.h"
#include "emit_reloc.h"
#include "emit_stream.h"
#include "core_fragment.h"
#include "core_symbol.h"
#include "core_addr.h"
#include "utility/print_object.h"

#include <boost/spirit/home/x3/support/utility/lambda_visitor.hpp>
#include <iostream>

#include "utility/print_type_name.h"

#if 0

The emit_expression needs `dot` because a core_expr could be unevaluated.
This is true for example for ``.long .-2``. Sizeof `.long` is known, so
expression arguments are never evaluated.

XXX no longer the case: .long (for example) evaluates via core_fits() which
XXX knows `dot`

XXX `dot` not needed for insns like `DWARF_LOC` which record location, instead
XXX of emiting data.

Processing of Expressions/Symbols.

Change `stream` interface to following:

put_ulong(chan, width, data) // more always false
put_symbol(chan, width, &sym, offset, as_displacement, more)
put_section(chan, width, &section, offset, as_displacement, more)
set_segment(&segment)

Refactor emit_expr as follows:

foreach (minus)
if (e == section)
    accumulate delta into offset, ++as_displacement
else
    error ("invalid expression")

foreach (plus)
put_symbol/section (chan, width, offset, as_displacement-- > 0, n)
offset = 0;

Refactor emit_ulong as:

put_ulong(chan, width, data+offset, as_displacement -- > 0, true)
offset = 0;


**********

Relocs


**********

#endif

namespace kas::core
{

// forward declare raw data manipulator
struct emit_data;

struct emit_base
{
    using result_type = void;       // for apply_visitor
    
    emit_base(emit_stream& stream) : stream(stream) {}

    // Entry point to emit an insn
    virtual void emit(struct core_insn& insn, core_expr_dot const& dot) = 0;
    virtual ~emit_base() = default;

    //
    // For each item streamed to `emit_base`, relocations and `width` bytes are emitted
    // to the object (or listing). For integral values streamed to `emit_base`, the size
    // can be infered from the object being streamed. For all others, the object size must
    // be specified (via a size manipulator) prior streaming the object.
    // hook to get at stream.put_filler();

    // supported types
    // integral types impute object size
    template <typename T, typename = std::enable_if_t<std::is_integral<T>::value>>
    emit_base& operator<<(T t)
    {
        put_fixed(sizeof(T), t);
        return *this;
    }

    emit_base& operator<<(expr_t const&);
    emit_base& operator<<(core_symbol const&);
    emit_base& operator<<(core_addr const&);
    void operator<<(parser::kas_diag const& diag);  // don't allow emit chain

    // XXX temp: diagnostic for listing 
    //emit_base& operator<<(std::string const& str);
    
    //
    // Section interface.
    //
    void set_segment(core_segment const& segment);
    std::size_t position() const;                   // for debug interface
    core_section const& get_section() const;        // for debug interface

    // XXX support manipulator functions...
    emit_base& operator<<(emit_base& (*fn)(emit_base&))
    {
        return fn(*this);
    }

    void set_width(std::size_t w);
    void set_chan (e_chan_num);

private:
    friend expr_t;          // for apply_visitor
    friend core_expr;       // implements special `emit` method.
    friend emit_data;
    friend emit_reloc_t;

    void assert_width() const;
    
    template <typename T, typename = std::enable_if_t<!std::is_integral<T>::value>>
    void operator()(T const&);

    template <typename T>
    std::enable_if_t<std::is_integral<T>::value> operator()(T);

    void put_fixed(std::size_t obj_width, uint64_t obj_data);
    
    void operator()(expr_t const& e);
    void operator()(core_addr const&);
    void operator()(core_symbol const&);
    void operator()(core_expr const&);
    void operator()(parser::kas_diag const&);
    
    // XXX temp for message in listing
    void operator()(std::string const&);

    void set_defaults();
    
    // push fixed data to stream, advancing position, reset defaults
    void emit_fixed();
    
    
    emit_stream&    stream;
    core_section const  *section_p {};
    
    e_chan_num      e_chan    { EMIT_DATA };
    int64_t         data      {};
    uint16_t        width     {};
};

//
// emit manipulators
//

// data size manipulator
struct set_size
{
    constexpr set_size(std::size_t w) : w(w) {}
private: 
    friend emit_base& operator<<(emit_base& b, set_size const& s)
    {
        b.set_width(s.w);
        return b;
    }

    std::size_t w;
};
    
static constexpr auto byte  = set_size(1);
static constexpr auto word  = set_size(2);
static constexpr auto _long = set_size(4);
static constexpr auto quad  = set_size(8);


// e_chan manipulators
template <e_chan_num N>
emit_base& _set_e_chan(emit_base& base)
{
    base.set_chan(N);
    return base;
}

static constexpr auto emit_expr = _set_e_chan<EMIT_EXPR>;
static constexpr auto emit_info = _set_e_chan<EMIT_INFO>;
static constexpr auto emit_addr = _set_e_chan<EMIT_ADDR>;

// name the "pc relative displacment" manipulator
using emit_disp = reloc_pcrel;

// raw data manipulator
struct emit_data
{
    uint16_t num_chunks;
    uint16_t chunk_size;
    emit_base *base_p;

    emit_data(uint16_t chunk_size, uint16_t num_chunks = 1)
        : chunk_size(chunk_size), num_chunks(num_chunks) {}

    auto& operator<<(void const* p)
    {
        base_p->stream.put_data(base_p->e_chan, p, chunk_size, num_chunks);
        base_p->set_defaults();
        return *base_p;
    }

    friend emit_data&& operator<<(emit_base& base, emit_data&& data)
    {
        data.base_p = &base;
        return std::move(data);
    }
};

// XXX 
struct emit_filler
{
    std::size_t num_bytes;
    uint64_t    fill_c;
    std::size_t fill_w;
    emit_base *base_p;

    emit_filler(std::size_t n, uint64_t fill_c = 0, std::size_t fill_w = sizeof(char))
        : num_bytes(n), fill_c(fill_c), fill_w(fill_w) {}

    // don't return emit_base& because resetting operation to defaults
    friend void operator<<(emit_base& base, emit_filler const& s)
    {
        // for now, just put N bytes of zero
        static constexpr uint64_t zero = 0;
        base << emit_data(1, s.num_bytes) << &zero;
    }

};

}



#endif
