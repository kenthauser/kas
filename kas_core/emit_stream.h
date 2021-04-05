#ifndef KAS_CORE_EMIT_STREAM_H
#define KAS_CORE_EMIT_STREAM_H

//
// Declare interface between `core_emit` and backends
//
// `core::emit_base` writes to this object
//

#include "kbfd/kbfd_object.h"
#include <cstdint>
#include <fstream>


namespace kas::core
{

// XXX
struct core_insn;
struct emit_base;
struct core_segment;

enum e_chan_num : uint16_t { EMIT_ADDR, EMIT_DATA, EMIT_EXPR, EMIT_INFO, NUM_EMIT_FMT };

struct emit_stream
{
    using emit_value_t = int64_t;       // value passed to emit backend

    // construct with optional `kbfd_object` ptr.
    // NB: `kbfd_object` required to emit relocations
    emit_stream(kbfd::kbfd_object *kbfd_p = {});
    emit_stream(kbfd::kbfd_object& obj) : emit_stream(&obj) {}

    // declare virtual dtor
    virtual ~emit_stream();
#if 0    
    // initialize/finalize backend for target object
    virtual void open (kbfd::kbfd_object&) {}
    virtual void close(kbfd::kbfd_object&) {}
#endif
    // default emit: drive `insn->emit`
    virtual void emit(core::core_insn& insn, core::core_expr_dot const *dot_p);
    
    // emit single value (with byte swapping)
    virtual void put_uint(e_chan_num num
                        , uint8_t    width
                        , emit_value_t data) = 0;

    // emit memory buffer (without byte swapping)
    virtual void put_raw(e_chan_num num
                       , void const *p
                       , uint8_t     size
                       , unsigned    count) = 0;
    
    // emit memory buffer (with byte swapping)
    virtual void put_data(e_chan_num num
                       , void const *p
                       , uint8_t     width 
                       , unsigned    count);
    
    // NB: if backend emits `REL_A` or otherwise consumes `addend`
    // it must zero addend
    virtual void put_symbol_reloc(
              e_chan_num num
            , kbfd::kbfd_target_reloc const& tgt_reloc
            , uint8_t offset
            , core_symbol_t const& sym
            , emit_value_t& addend
            ) = 0;
    virtual void put_section_reloc(
              e_chan_num num
            , kbfd::kbfd_target_reloc const& tgt_reloc
            , uint8_t offset
            , core_section const& section
            , emit_value_t& addend
            ) = 0;

    // access `core_base` (throws if not defined)
    auto& base()
    {
        if (base_p)
            return *base_p;
        throw std::logic_error("emit_stream::base: base undefined");
    };

    // emit diagnostics
    virtual void put_diag(e_chan_num, uint8_t, parser::kas_diag_t const&) = 0;
    
    // current section interface
    virtual void set_section(core_section const&) = 0;
    virtual std::size_t position() const = 0;
   
    // convience methods
    void set_segment(core_segment const& segment);


protected:
    kbfd::kbfd_object *kbfd_p;
    emit_base *base_p;
};
    
struct emit_fstream : emit_stream
{
    // ctor using ostream&
    emit_fstream(kbfd::kbfd_object&, std::ostream& out);
    
    // ctor using path: allocate file (ie ofstream object)
    emit_fstream(kbfd::kbfd_object&, const char *path);
    
    // ctor: allow string for path
    emit_fstream(kbfd::kbfd_object&, std::string const& path);

    // dtor: close file if allocated
    ~emit_fstream();

private:
    // NB: `file_p` must preceed `out` for proper initialization
    std::ofstream *file_p {};

protected:
    std::ostream& out;
};

}


#endif
