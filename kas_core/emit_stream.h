#ifndef KAS_CORE_EMIT_STREAM_H
#define KAS_CORE_EMIT_STREAM_H

//
// Declare interface between `core_emit` and backends
//
// `core::emit_base` writes to this object
//

#include <cstdint>
#include "core_reloc.h"


namespace kas::core
{

enum e_chan_num : uint16_t { EMIT_ADDR, EMIT_DATA, EMIT_EXPR, EMIT_INFO, NUM_EMIT_FMT };

struct emit_stream
{
    using emit_value_t = int64_t;       // value passed to emit backend

    // emit single value
    virtual void put_uint(e_chan_num, uint8_t width, emit_value_t data) = 0;

    // emit count of size (in bytes) values from buffer
    virtual void put_data(e_chan_num, void const *, uint8_t size, uint8_t count) = 0;
    
    // emit reloc
    virtual void put_symbol_reloc(
              e_chan_num num
            , kbfd::kas_reloc_info const& info
            , uint8_t offset
            , core_symbol_t const& sym
            , emit_value_t& addend
            ) = 0;
    virtual void put_section_reloc(
              e_chan_num num
            , kbfd::kas_reloc_info const& info
            , uint8_t offset
            , core_section const& section
            , emit_value_t& addend
            ) = 0;

    // emit diagnostics
    virtual void put_diag(e_chan_num, uint8_t, parser::kas_diag_t const&) = 0;
    
    // current section interface
    virtual void set_section(core_section const&) = 0;
    virtual std::size_t position() const = 0;

    virtual ~emit_stream() = default;
};

}


#endif
