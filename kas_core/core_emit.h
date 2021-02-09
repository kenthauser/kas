#ifndef KAS_CORE_CORE_EMIT_H
#define KAS_CORE_CORE_EMIT_H

// `core_emit` is the interface between the assembler `front-end` (where source is interpreted)
// and the `back-end` (where output is generated).
//
// 

#include "expr/expr.h"
#include "core_reloc.h"
#include "emit_stream.h"
#include "core_fragment.h"
#include "core_symbol.h"
#include "core_addr.h"
#include "utility/print_object.h"

#include <boost/spirit/home/x3/support/utility/lambda_visitor.hpp>
#include <iostream>

#include "utility/print_type_name.h"

namespace kas::core
{

// forward declare manipulators
struct set_size;
struct emit_reloc;
struct emit_disp;
struct emit_data;

struct emit_base
{
    using result_type  = void;       // for apply_visitor
    using emit_value_t = typename emit_stream::emit_value_t;
    
    emit_base(emit_stream& stream) : stream(stream) {} 

    // Entry point to create instruction output
    // NB: `dot_p` not to be used to calculate size (via `fits`).
    // NB: `dot_p` can be used to tag location in dwarf `opc_dw_line`,
    // NB: for address in listings, and for other debugging facilities
    virtual void emit(struct core_insn& insn, core_expr_dot const *dot_p = {}) = 0;
    virtual ~emit_base() = default;

    // For each item streamed to `emit_base`, relocations and `width` bytes are emitted
    // to the object (or listing). For integral values streamed to `emit_base`, the size
    // can be infered from the object being streamed. For all others, the object size must
    // be specified (via a size manipulator) prior to streaming the object.

    // supported types:

    // integral types impute object size
    // also integral types never emit diagnostics, so no `kas_loc` needed
    template <typename T
            , typename = std::enable_if_t<std::is_integral<T>::value>
            , typename = std::enable_if_t<sizeof(T) <= sizeof(uint32_t)>
            >
    emit_base& operator<<(T t)
    {
        put_fixed(t, sizeof(T));
        return *this;
    }

    // general case emit: accept location-tagged expression
    emit_base& operator<<(expr_t const& value)
        { (*this)(value); return *this; }

    // special case: emits from internal sources (eg dwarf)
    emit_base& operator<<(core_symbol_t const& value)
        { (*this)(value); return *this; }
    emit_base& operator<<(core_addr_t   const& value)
        { (*this)(value); return *this; }
    emit_base& operator<<(core_expr_t   const& value)
        { (*this)(value); return *this; }

    // special case: don't allow diagnostics to chain
    void operator<<(parser::kas_diag_t const& diag)
        { (*this)(diag); }

    // support manipulator functions..
    emit_base& operator<<(emit_base& (*fn)(emit_base&))
    {
        return fn(*this);
    }
    
    //
    // section interface
    //
    
    void set_segment(core_segment const& segment);
    std::size_t position() const;
    core_section const& get_section() const;


private:
    friend expr_t;          // for apply_visitor
    friend core_expr_t;     // implements special `emit` method.

    // friend manipulators
    friend set_size;
    friend emit_reloc;
    friend emit_disp;
    friend emit_data;
    friend deferred_reloc_t;
    //friend emit_reloc_t;

    // driven entry-points from stream operators above 
    void operator()(expr_t const& e);
    void operator()(core_addr_t const&, kas_loc const * = {});
    void operator()(core_symbol_t const&, kas_loc const * = {});
    void operator()(core_expr_t const&, kas_loc const * = {});
    void operator()(parser::kas_diag_t const&, kas_loc const * = {});

    // unsupported
    template <typename T>
    void operator()(T const&, kas_loc const * = {});
    
    // set width & emit object code
    void put_fixed(int64_t value, uint8_t obj_width = {});
    
    // apply relocs, push fixed data to stream (advancing position), reset defaults
    void emit_obj_code();
public:
    void set_chan (e_chan_num);
    kbfd::elf_reloc_t const *elf_reloc_p {};
private:

    // emit relocations (used by `core_reloc`)
    void put_section_reloc(deferred_reloc_t const&, kbfd::kbfd_target_reloc const *info_p
                         , core_section const& section, int64_t& addend);
    void put_symbol_reloc (deferred_reloc_t const&, kbfd::kbfd_target_reloc const *info_p
                         , core_symbol_t  const& symbol, int64_t& addend);
    
    // manipulators to configure data stream
    void set_width(std::size_t w);

    // utility methods
    deferred_reloc_t& add_reloc(kbfd::kbfd_reloc r = {}, int64_t addend = {}, uint8_t offset = {});

    void assert_width() const;      
    void set_defaults();
    
    // instance data
    emit_stream&         stream;
    core_section const  *section_p {};
    
    // save pending relocs in array
    static constexpr auto MAX_RELOCS_PER_LOCATION = 4;
    std::array<deferred_reloc_t, MAX_RELOCS_PER_LOCATION> relocs{};

    emit_value_t         data      {};
    e_chan_num      e_chan    { EMIT_DATA };
    uint8_t         width     {};
    deferred_reloc_t::flags_t reloc_flags;
    deferred_reloc_t *reloc_p { relocs.begin() };
};

//
// `core_emit` stream manipulators
//

// data size manipulator
struct set_size
{
    constexpr set_size(uint8_t w) : w(w) {}
private: 
    friend emit_base& operator<<(emit_base& b, set_size const& s)
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
emit_base& _set_e_chan(emit_base& base)
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
    using emit_value_t = typename emit_base::emit_value_t;


    emit_reloc(kbfd::kbfd_reloc r, emit_value_t addend = {}, uint8_t offset = {})
        : reloc(r), addend(addend), offset(offset) {}
    
    // expect relocatable expression.
    auto& operator<<(expr_t const& e)
    {
        (*r)(e);
        return *base_p;
    }
    
private:
    friend auto operator<<(emit_base& base, emit_reloc r)
    {
        r.base_p = &base;
        r.r      = &base.add_reloc(r.reloc, r.addend, r.offset);
        return r;
    }

    emit_base *base_p;
    deferred_reloc_t *r {};

    // save ctor values
    kbfd::kbfd_reloc reloc;
    emit_value_t     addend;
    uint8_t          offset;
};

// emit relocation for displacement from current location (with size & offset)
struct emit_disp
{
    using emit_value_t = typename emit_base::emit_value_t;
    
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
    friend auto operator<<(emit_base& base, emit_disp r)
    {
        kbfd::kbfd_reloc reloc { kbfd::K_REL_ADD()
                             , static_cast<uint8_t>(r.size * 8)
                             , true };
        
        r.base_p = &base;
        r.r_p    = &base.add_reloc(reloc, r.addend, r.offset);
        return r;
    }

    emit_base *base_p;
    deferred_reloc_t *r_p;
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
    friend emit_data&& operator<<(emit_base& base, emit_data&& data)
    {
        data.base_p = &base;
        return std::move(data);
    }
    
    emit_base *base_p;
    uint16_t num_chunks;
    uint16_t chunk_size;
};

// filler data manipulator: usage example: .fill & .align
// NB: not a `core_emit` friend. Use `emit_data` interface
struct emit_filler
{
    using emit_value_t = typename emit_base::emit_value_t;
    
    emit_filler(uint8_t n, emit_value_t fill_c = 0, uint8_t fill_w = sizeof(char))
        : num_bytes(n), fill_c(fill_c), fill_w(fill_w) {}

private:
    // don't return emit_base& because resetting operation to defaults
    friend void operator<<(emit_base& base, emit_filler const& s)
    {
        // for now, just put N bytes of zero
        static constexpr emit_value_t zero = 0;
        base << emit_data(sizeof(char), s.num_bytes) << &zero;
    }

    emit_base   *base_p;
    emit_value_t fill_c;
    uint8_t      fill_w;
    uint8_t      num_bytes;
};

}



#endif
