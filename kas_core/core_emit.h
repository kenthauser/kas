#ifndef KAS_CORE_CORE_EMIT_H
#define KAS_CORE_CORE_EMIT_H

// `core_emit` is the interface between the assembler `front-end` (where source is interpreted)
// and the `back-end` (where output is generated).

#include "expr/expr.h"
#include "emit_stream.h"
#include "core_reloc.h"
#include "core_fragment.h"
#include "core_symbol.h"
#include "core_addr.h"
#include "utility/print_object.h"

#include <boost/spirit/home/x3/support/utility/lambda_visitor.hpp>
#include <iostream>

#include "utility/print_type_name.h"

namespace kas::core
{
// forward declare "emit" stream manipulators
struct set_size;
struct emit_reloc;
struct emit_disp;
struct emit_data;

struct core_emit
{
    using result_type  = void;       // for apply_visitor
    using emit_value_t = typename emit_stream_base::emit_value_t;
    
    // housekeeping methods: ctor, init, dtor
    // NB: `obj_p` required if emitting relocations
    core_emit(emit_stream_base& stream, kbfd::kbfd_object *obj_p = {})
        : stream(stream), obj_p(obj_p)
    {
        set_defaults();
    }

    // For each item streamed to `core_emit`, relocations and `width` bytes are emitted
    // to the object (or listing). For integral values streamed to `core_emit`, the size
    // can be inferred from the object being streamed. For all others, the object size must
    // be specified (via a size manipulator) prior to streaming the object.

    // supported types:

    // integral types impute object size
    // also integral types never emit diagnostics, so no `kas_loc` needed
    template <typename T
            , typename = std::enable_if_t<std::is_integral<T>::value>
            , typename = std::enable_if_t<sizeof(T) <= sizeof(uint32_t)>
            >
    core_emit& operator<<(T t)
    {
        put_fixed(t, sizeof(T));
        return *this;
    }

    // general case emit: accept location-tagged expression
    core_emit& operator<<(expr_t const& value)
        { (*this)(value); return *this; }

    // special case: emits from internal sources (eg dwarf)
    core_emit& operator<<(core_symbol_t const& value)
        { (*this)(value); return *this; }
    core_emit& operator<<(core_addr_t   const& value)
        { (*this)(value); return *this; }
    core_emit& operator<<(core_expr_t   const& value)
        { (*this)(value); return *this; }

    // special case: don't allow diagnostics to chain
    void operator<<(parser::kas_diag_t const& diag)
        { (*this)(diag); }

    // support manipulator functions..
    core_emit& operator<<(core_emit& (*fn)(core_emit&))
    {
        return fn(*this);
    }
    
    // driven entry-points from stream operators above 
    void operator()(expr_t const& e);
    void operator()(core_addr_t const&, kas_loc const * = {});
    void operator()(core_symbol_t const&, kas_loc const * = {});
    void operator()(core_expr_t const&, kas_loc const * = {});
    void operator()(parser::kas_diag_t const&, kas_loc const * = {});

    // emit unsupported value (NB: will result in error)
    template <typename T>
    void operator()(T const&, kas_loc const * = {});
    
    // retrive & clear error state
    e_diag_t const *get_error()
    {
        auto e = error_p; error_p = {}; return e;
    }
   
    //
    // section interface
    //
    
    void set_segment(core_segment const& segment);
    std::size_t position() const;
    core_section const& get_section() const;

    // set emit "channel"
    void set_chan (e_chan_num);

private:
    // be-friend emit stream manipulators
    friend set_size;
    friend emit_reloc;
    friend emit_disp;
    friend emit_data;

    // set width & emit object code
    void put_fixed(int64_t value, uint8_t obj_width = {});
    
    // apply relocs, push fixed data to stream (advancing position), reset defaults
    void emit_obj_code();

    // emit relocations (used by `core_reloc`)
    friend core_reloc;

    void put_reloc(core_reloc&, core_section  const& section);
    void put_reloc(core_reloc&, core_symbol_t const& symbol);
    void put_reloc(core_reloc&);
    
    // utility methods
    void set_width(std::size_t w);
    void assert_width() const;      
    void set_defaults();

    // record relocation for emit
    core_reloc& add_reloc(core_reloc&&);

    // support routines to select proper target relocation
    // XXX need to tune...
    core_symbol_t const* reloc_select_base_sym(core_symbol_t const&) const
    {
        return {};      // use segment
    }
    
    kbfd::kbfd_target_reloc const *get_tgt_reloc(core_reloc&) const;
    

    template <typename...Ts>
    core_reloc& add_reloc(Ts&&...args)
    {
        return add_reloc(core_reloc(std::forward<Ts>(args)...));
    }

    // utility to translate `reloc` to appropriate target format
    kbfd::kbfd_target_reloc const *get_reloc(kbfd::kbfd_reloc&) const;

    // save pending relocs in array
    static constexpr auto MAX_RELOCS_PER_LOCATION = 4;
    std::array<core_reloc, MAX_RELOCS_PER_LOCATION> relocs{};

    // instance data
    emit_stream_base&   stream;
    kbfd::kbfd_object  *obj_p {};
    core_section const *section_p {};
    e_diag_t const     *error_p   {};   // current `emit` error state
    
    // info about current data being emitted. initialized by `set_defaults()`
    core_reloc         *reloc_p;    // pointer to "next" reloc for insn
    emit_value_t        data;       // base value being emitted
    e_chan_num          e_chan;     // output channel
    uint8_t             width;      // width of `data` in bytes
};

//
// `core_emit` stream manipulators
//

// data size manipulator
struct set_size
{
    constexpr set_size(uint8_t w) : w(w) {}
private: 
    friend core_emit& operator<<(core_emit& b, set_size const& s)
    {
        b.set_width(s.w);
        return b;
    }

    uint8_t w;
};
    
static constexpr auto byte  = set_size(1);
static constexpr auto word  = set_size(2);
static constexpr auto _long = set_size(4);
static constexpr auto quad  = set_size(8);


// e_chan manipulators
template <e_chan_num N>
core_emit& _set_e_chan(core_emit& base)
{
    base.set_chan(N);
    return base;
}

static constexpr auto emit_expr = _set_e_chan<EMIT_EXPR>;
static constexpr auto emit_info = _set_e_chan<EMIT_INFO>;
static constexpr auto emit_addr = _set_e_chan<EMIT_ADDR>;

// emit relocation manipulator
struct emit_reloc
{
    using emit_value_t = typename core_emit::emit_value_t;
    struct flush {};

    emit_reloc(kbfd::kbfd_reloc r, emit_value_t addend = {}
             , uint8_t offset = {}, uint8_t r_flags = {})
        : reloc(r), addend(addend), offset(offset), r_flags(r_flags) {}
    
    // expect relocatable expression.
    auto& operator<<(expr_t const& e)
    {
        (*r_p)(e);
        return *base_p;
    }

    auto& operator<<(flush const&)
    {
        (*r_p)(0, core_reloc::CR_EMIT_BARE);
        return *base_p;
    }
    
private:
    friend auto operator<<(core_emit& base, emit_reloc r)
    {
        r.base_p = &base;
        r.r_p    = &base.add_reloc(r.reloc, r.addend, r.offset, r.r_flags);
        return r;
    }

    core_emit    *base_p;
    core_reloc *r_p {};

    // save ctor values
    kbfd::kbfd_reloc reloc;
    emit_value_t     addend;
    uint8_t          offset;
    uint8_t          r_flags;
};

// emit relocation for displacement from current location (with size & offset)
struct emit_disp
{
    using emit_value_t = typename core_emit::emit_value_t;
    
    // declare size in bytes, offset from current location
    emit_disp(uint8_t size, emit_value_t addend = {}, emit_value_t offset = {})
        : size(size), addend(addend), offset(offset) {}

    // expect relocatable expression.
    auto& operator<<(expr_t const& e)
    {
        (*r_p)(e);
        return *base_p << set_size(size) << 0;
    }

private:
    friend auto operator<<(core_emit& base, emit_disp r)
    {
        // only lookup `K_REL_ADD` once (mark as PC_relative)
        static kbfd::kbfd_reloc _proto { kbfd::K_REL_ADD(), 0, true };

        // copy prototype & set width
        auto reloc = _proto;
        _proto.default_width(r.size * 8);
        
        r.base_p = &base;
        r.r_p    = &base.add_reloc(reloc, r.addend, r.offset);
        return r;
    }

    core_emit *base_p;
    core_reloc *r_p;
    emit_value_t addend, offset;
    uint8_t      size;
};


// raw data manipulator: usage example: floating point constant
struct emit_data
{
    emit_data(uint16_t chunk_size, uint16_t num_chunks = 1)
        : chunk_size(chunk_size), num_chunks(num_chunks) {}

    auto& operator<<(void const* p)
    {
        base_p->stream.put_data(base_p->e_chan, p, chunk_size, num_chunks);
        base_p->set_defaults();
        return *base_p;
    }

private:
    friend emit_data&& operator<<(core_emit& base, emit_data&& data)
    {
        data.base_p = &base;
        return std::move(data);
    }
    
    core_emit *base_p;
    uint16_t num_chunks;
    uint16_t chunk_size;
};

// filler data manipulator: usage example: .fill & .align
// NB: not a `core_emit` friend. Use `emit_data` interface
struct emit_filler
{
    using emit_value_t = typename core_emit::emit_value_t;
    
    emit_filler(uint8_t n, emit_value_t fill_c = 0, uint8_t fill_w = sizeof(char))
        : num_bytes(n), fill_c(fill_c), fill_w(fill_w) {}

private:
    // don't return core_emit& because resetting operation to defaults
    friend void operator<<(core_emit& base, emit_filler const& s)
    {
        // for now, just put N bytes of zero
        static constexpr emit_value_t zero = 0;
        base << emit_data(sizeof(char), s.num_bytes) << &zero;
    }

    core_emit   *base_p;
    emit_value_t fill_c;
    uint8_t      fill_w;
    uint8_t      num_bytes;
};

}



#endif
