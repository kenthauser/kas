#ifndef KAS_CORE_EMIT_STREAM_H
#define KAS_CORE_EMIT_STREAM_H


//
// Declare interface between `core_emit` and backends
//

#include <cstdint>
#include "core_reloc.h"


namespace kas::core
{

enum e_chan_num : uint16_t { EMIT_ADDR, EMIT_DATA, EMIT_EXPR, EMIT_INFO, NUM_EMIT_FMT };

struct emit_stream
{
    using emit_value_t = int64_t;       // value passed to emit backend

    // emit data
    virtual void put_uint(e_chan_num num, uint8_t width, emit_value_t data) = 0;
    virtual void put_data(e_chan_num num, void const *, uint8_t chunk_size, uint8_t num_chunks) = 0;
    
    // emit reloc
    virtual void put_symbol_reloc(
              e_chan_num num
            , reloc_info_t const& info
            , uint8_t width
            , uint8_t offset
            , core_symbol const& sym
            , emit_value_t addend
            ) = 0;
    virtual void put_section_reloc(
              e_chan_num num
            , reloc_info_t const& info
            , uint8_t width
            , uint8_t offset
            , core_section const& section
            , emit_value_t addend
            ) = 0;

    // emit diagnostics
    virtual void put_diag(e_chan_num, uint8_t, parser::kas_diag const&) = 0;

    // XXX emit temp diagnostic message (type not known)
    virtual void put_str(e_chan_num, uint8_t, std::string const&) = 0;
    
    // current section interface
    virtual void set_section(core_section const&) = 0;
    virtual std::size_t position() const = 0;

    virtual ~emit_stream() = default;
};

}


#endif
