#ifndef KAS_CORE_EMIT_STREAM_H
#define KAS_CORE_EMIT_STREAM_H


//
// Declare interface between `core_emit` and backends
//

#include <cstdint>


namespace kas::core
{

enum e_chan_num : uint16_t { EMIT_ADDR, EMIT_DATA, EMIT_EXPR, EMIT_INFO, NUM_EMIT_FMT };

struct emit_stream
{
    // emit data
    virtual void put_uint(e_chan_num num, std::size_t width, uint64_t data) = 0;
    virtual void put_data(e_chan_num num, void const *, std::size_t chunk_size, std::size_t num_chunks) = 0;
    
    // emit reloc
    virtual void put_symbol_reloc(
              e_chan_num num
            , uint32_t    reloc
            , core_symbol const& sym
            , int64_t& data
            ) = 0;
    virtual void put_section_reloc(
              e_chan_num num
            , uint32_t    reloc
            , core_section const& section
            , int64_t& data
            ) = 0;

    // emit diagnostics
    virtual void put_diag(e_chan_num, std::size_t, parser::kas_diag const&) = 0;

    // XXX emit temp diagnostic message (type not known)
    virtual void put_str(e_chan_num, std::size_t, std::string const&) = 0;
    
    // current section interface
    virtual void set_section(core_section const&) = 0;
    virtual std::size_t position() const = 0;

    virtual ~emit_stream() = default;
};

}


#endif
