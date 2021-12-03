#ifndef KAS_CORE_EMIT_KBFD_H
#define KAS_CORE_EMIT_KBFD_H

// emit_kbfd.h
//
// the `emit_kbfd` object is the interface between `core_emit` object
// and the object file backend. 
//
// `kbfd` supports various file formats, but presents a common interface.
// the `emit_kbfd` module is initialized with a `kbfd_object` which
// formats the binary data as appropriate for file format.


#include "emit_stream.h"
#include "kbfd/kbfd_object.h"

namespace kas::core
{

struct emit_kbfd : emit_stream
{
    // initialize `kbfd` after allocating `stream`
    emit_kbfd(kbfd::kbfd_object& kbfd, std::ostream& out)
        : emit_stream(kbfd, out)
    {
        open();
    }

    // flush `kbfd` data before deletion
    ~emit_kbfd()
    {
        close();
    }

    // initialize symbol table & data sections
    void open();

    // write data to output stream
    void close();

private:
    // filter out `e_chan_num` values which are not sent to `kbfd` backend
    static constexpr bool do_emit(uint8_t chan)
    {
        switch(chan)
        {
            case EMIT_DATA:
                return true;

            default:
                return false;
        }
    }

public:
    void put_uint(e_chan_num num, uint8_t width, int64_t data) override;
    
    void put_raw(e_chan_num num
                , void const *data_p
                , uint8_t chunk_size
                , unsigned num_chunks) override;
    
    void put_data(e_chan_num num
                , void const *data_p
                , uint8_t chunk_size
                , unsigned num_chunks) override;

    void put_symbol_reloc(
                  e_chan_num num
                , kbfd::kbfd_target_reloc const& info
                , uint8_t offset
                , core_symbol_t const& sym
                , int64_t& addend
                ) override
    {
        auto sym_num = sym.sym_num();
        if (!sym_num)
            throw std::logic_error("emit_kbfd: no sym_num for symbol: " + sym.name());
        put_kbfd_reloc(num, info, sym_num, offset, addend);
    }

    void put_section_reloc(
                  e_chan_num num
                , kbfd::kbfd_target_reloc const& info
                , uint8_t offset
                , core_section const& section
                , int64_t& addend 
                ) override;

    void put_bare_reloc(
                  e_chan_num num
                , kbfd::kbfd_target_reloc const& info
                , uint8_t offset
                ) override
    {
        int64_t dummy;
        put_kbfd_reloc(num, info, 0, offset, dummy);
    }

    void put_diag(e_chan_num num
                , uint8_t width
                , parser::kas_diag_t const& diag) override;

    // section control
    // utilize callback in `core_section` to xlate section
    void set_section(core_section const& s) override
    {
        ks_data_p = &core2ks_data(s);
    }
    
    std::size_t position() const override;


private:
    // construct or retrive reference to `kbfd` ks_data object
    kbfd::ks_data& core2ks_data(core_section const&) const;

    // create ELF symbols from `core` structures
    void add_sym(core_symbol_t& s) const;
    void add_sym(core_section & s) const;

    // utility to decide if symbol should be emitted...
    bool should_emit_local(core_symbol_t& s) const;
    bool should_emit_non_local(core_symbol_t& s) const;

    // put opaque memory block into data segment
    void put(e_chan_num num, void const *p, uint8_t width);

    // actually emit a reloc
    void put_kbfd_reloc(
                  e_chan_num num
                , kbfd::kbfd_target_reloc const& info 
                , uint32_t sym_num
                , uint8_t  offset
                , int64_t& data
                ) const;

    // NB: first directive in `core_emit` is `set_section`
    kbfd::ks_data   *ks_data_p{};    // current section
};

}


#endif
