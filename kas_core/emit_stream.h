#ifndef KAS_CORE_EMIT_STREAM_H
#define KAS_CORE_EMIT_STREAM_H

//
// Declare interface between `core_emit` and backends
//
// `core::emit_base` writes to this object
//

#include <cstdint>
#include "kbfd/kbfd_object.h"


namespace kas::core
{

enum e_chan_num : uint16_t { EMIT_ADDR, EMIT_DATA, EMIT_EXPR, EMIT_INFO, NUM_EMIT_FMT };

struct emit_stream
{
    using emit_value_t = int64_t;       // value passed to emit backend

    // initialize/finalize backend for target object
    virtual void open (kbfd::kbfd_object&) {}
    virtual void close(kbfd::kbfd_object&) {}

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
                       , unsigned    count)
    {
        auto get_as_width = [width](auto p) -> uint64_t
            {
                // NB: if values properly aligned when put in "buffer"
                // they will be properly aligned when extracted
                switch (width)
                {
                    case 8:
                        return *static_cast<uint64_t const *>(p);
                    case 4:
                        return *static_cast<uint32_t const *>(p);
                    case 2:
                        return *static_cast<uint16_t const *>(p);
                    case 1:
                        return *static_cast<uint8_t  const *>(p);
                    default:
                        throw std::logic_error("emit_stream::put_data: invalid width");
                }
            };
        
        while (count--)
        {
            put_uint(num, width, get_as_width(p));
            p = static_cast<const char *>(p) + width;
        }
    }
    
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

    // emit diagnostics
    virtual void put_diag(e_chan_num, uint8_t, parser::kas_diag_t const&) = 0;
    
    // current section interface
    virtual void set_section(core_section const&) = 0;
    virtual std::size_t position() const = 0;

    virtual ~emit_stream() = default;
};

}


#endif
