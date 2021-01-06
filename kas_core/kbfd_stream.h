#ifndef KAS_CORE_KBFD_STREAM_H
#define KAS_CORE_KBFD_STREAM_H

// kbfd_stream.h
//
// the `kbfd_stream` object is the interface between `kas_core_emit`
// and the object file backend. 
//
// Information from the assembler passed to backend using `elf` semantics.
// Thus the symbol table entries encode such things as `GLOBAL` using `elf`
// constants. The relocations also use the `elf` semantic of pseudo-symbol for
// section relocation, though the `relocation code` is appropriate for target
// backend (eg: coff backends get coff constants). Object data is passed as 
// "host" variables (ie without endian)
//
// The `elf` module holds all headers in ELF64 format, host endian.
// All data stored in section data is stored in target endian. The
// symbol table is saved in both ELF64-host-endian and target formats
// so that the symbol table can be easily referenced during assembly.
//
// This module performs byte-swapping for endian conversion, and 
// utilizes the `kbfd_convert` module to translate symbols and relocations
// to target format.
// 
// NB: though the header format is ELF64, the values in the header reflect
// the actual content. For instance, for a relocation section, the "sh_entsize"
// value will reflect the stored relocation size, not the size of a `Elf64_Rela`
// relocation. 


#include "emit_stream.h"
#include "kbfd/kbfd_endian.h"
#include "kbfd/kbfd_convert.h"
#include "kbfd/kbfd_object.h"
#include "kbfd/kbfd_section_sym.h"
#include "kbfd/kbfd_section_data.h"

#include "kbfd/kbfd_convert_elf.h"    // host format

namespace kas::core
{

struct kbfd_stream : emit_stream
{
    // used for swap endian (both headers & data)
    //static auto constexpr target_endian  = KBFD_FORMAT::endian;
    
    // get target header formats
    //using target_headers = typename KBFD_FORMAT::headers;

    // init from assembler
    template <typename KBFD_OBJECT>
    kbfd_stream(KBFD_OBJECT& object);
            
public:
    void put_uint(e_chan_num num, uint8_t width, int64_t data) override
    {
        // pass value, not pointer
        put(num, swap(data, width), width);
    }

    void put_data(e_chan_num num
                , void const *data_p
                , uint8_t chunk_size
                , uint8_t num_chunks) override
    {
        switch(chunk_size)
        {
        case 0:
            // short circuit null case
            break;

        case 1:
            // short circut simple (byte) case
            put(num, data_p, num_chunks);
            break;

        default:
            {
                // for each chunk in sequence, read, swap endian, & emit
                // NB: `swap` advances `data_p` as appropriate
                while (num_chunks--)
                    put(num, swap(data_p, chunk_size), chunk_size);
                break;
            }
        }
    }

    void put_symbol_reloc(
                  e_chan_num num
                , kbfd::kas_reloc_info const& info
                , uint8_t offset
                , core_symbol_t const& sym
                , int64_t& addend
                ) override
    {
        auto sym_num = sym.sym_num();
        if (!sym_num)
            throw std::logic_error("kbfd_stream: no sym_num for symbol: " + sym.name());
        put_kbfd_reloc(num, info, sym_num, offset, addend);
    }

    void put_section_reloc(
                  e_chan_num num
                , kbfd::kas_reloc_info const& info
                , uint8_t offset
                , core_section const& section
                , int64_t& addend 
                ) override
    {
        auto sym_num = core2ks_data(section).sym_num;
        put_kbfd_reloc(num, info, sym_num, offset, addend);
    }

    void put_diag(e_chan_num num, uint8_t width, parser::kas_diag_t const& diag) override
    {
        static constexpr char zero[8] = {};
        // diagnostics not emitted into object data.
        // emit `width` of zeros
        put(num, zero, width);
    }

    // section control
    // utilize callback in `core_section` to xlate section
    void set_section(core_section const& s) override
    {
        ks_data_p = &core2ks_data(s);
    }
    
    std::size_t position() const override
    {
        return ks_data_p->position();
    }


private:
    // data section ctor
    kbfd::ks_data& core2ks_data(core_section const&) const;

    // create ELF symbols from `core` structures
    void add_sym(core_symbol_t& s) const;
    void add_sym(core_section & s) const;

    // utility to decide if symbol should be emitted...
    bool should_emit_local(core_symbol_t& s) const;
    bool should_emit_non_local(core_symbol_t& s) const;

    // put opaque memory block into data segment
    void put(e_chan_num num, void const *p, uint8_t width)
    {
        if (num == EMIT_DATA)
            ks_data_p->put(p, width);
    }

    // actually emit a reloc
    void put_kbfd_reloc(
                  e_chan_num num
                , kbfd::kas_reloc_info const& info 
                , uint32_t sym_num
                , uint8_t  offset
                , int64_t& data
                ) const;

    // NB: `core_emit` first directive is `set_section`
    kbfd::ks_data   *ks_data_p{};    // current section

    kbfd::swap_endian const& swap;   // convenience: grab referece to object::swap
    kbfd::kbfd_object& object;        // holds kbfd_section modules
};

}


#endif
