#ifndef KAS_ELF_ELF_STREAM_H
#define KAS_ELF_ELF_STREAM_H

#include "kas_core/core_emit.h"
#include "elf_file.h"
#include "elf_symbol_util.h"

namespace kas::elf
{

struct elf_stream : core::emit_stream
{
    template <typename...Ts>
    constexpr elf_stream(Ts&&...args)
            : elf_output{std::forward<Ts>(args)...}
    {
        // XXX need to move init
        es_data_p = &elf_output.data_sections[0];
    }
            
private:
    void const *swap(void const *p, std::size_t width) const
    {
        auto& cvt = elf_output.cvt;
        switch(width)
        {
            case 8:
            {
                static uint64_t data64;
                data64 = cvt.swap(*static_cast<uint64_t const *>(p));
                return &data64;
            }
            case 4:
            {
                static uint32_t data32;
                data32 = cvt.swap(*static_cast<uint32_t const *>(p));
                return &data32;
            }
            case 2:
            {
                static uint16_t data16;
                data16 = cvt.swap(*static_cast<uint16_t const *>(p));
                return &data16;
            }
            case 1:
            {
                return p;
            }
            case 0:
            {
                return nullptr;
            }
            default:
                throw std::logic_error("elf_stream::swap: invalid width: "
                                                + std::to_string(width));
        }
    }

    void put(core::e_chan_num num, void const *p, std::size_t width)
    {
        if (num == core::EMIT_DATA)
            es_data_p->put(p, width);
    }


public:
    void put_uint(core::e_chan_num num, std::size_t width, uint64_t data) override
    {
        put(num, swap(&data, width), width);
    }

    void put_data(core::e_chan_num num, void const *data_p, std::size_t chunk_size, std::size_t num_chunks) override
    {
        if (num != core::EMIT_DATA)
            return;

        switch(chunk_size) {
        case 0:
            return;

        case 1:
            // short circut simple case
            return put(num, static_cast<uint8_t const *>(data_p), num_chunks);
        case 2:
        {
            // read, then swap endian, each chunk in sequence
            auto p = static_cast<uint16_t const *>(data_p);
            while (num_chunks--)
                put(num, swap(p++, 2), 2);
            break;
        }
        case 4:
        {
            // read, then swap endian, each chunk in sequence
            auto p = static_cast<uint32_t const *>(data_p);
            while (num_chunks--)
                put(num, swap(p++, 4), 4);
            break;
        }
        case 8:
        {
            // read, then swap endian, each chunk in sequence
            auto p = static_cast<uint64_t const *>(data_p);
            while (num_chunks--)
                put(num, swap(p++, 8), 8);
            break;
        }
        default:
        {
            throw std::logic_error(
                "elf_stream::put_data: invalid size "
                        + std::to_string(chunk_size)
                );
        }
        }
    }

    void put_elf_reloc(
                  core::e_chan_num num
                , uint32_t reloc
                , uint32_t sym_num
                , int64_t& data
                ) 
    {
        static constexpr auto use_rel_a = true;
        
        if (num != core::EMIT_DATA)
            return;

        if (data || use_rel_a)
            es_data_p->put_reloc_a(reloc, sym_num, data);
        else
            es_data_p->put_reloc(reloc, sym_num);
    }

    void put_symbol_reloc(
                  core::e_chan_num num
                , core::reloc_info_t const& info
                , uint8_t width
                , uint8_t offset
                , core::core_symbol const& sym
                , int64_t addend
                ) override
    {
        auto sym_num = sym.sym_num();
        if (!sym_num)
            throw std::logic_error("emit_elf: no sym_num for symbol: " + sym.name());
        put_elf_reloc(num, info.num, sym_num, addend);
    }

    void put_section_reloc(
                  core::e_chan_num num
                , core::reloc_info_t const& info
                , uint8_t width
                , uint8_t offset
                , core::core_section const& section
                , int64_t addend 
                ) override
    {
        auto sym_num = es_data::core2es(section).sym_num;
        put_elf_reloc(num, info.num, sym_num, addend);
    }

    // put_str & put_diag are used in generation of listings...
    void put_str(core::e_chan_num num, std::size_t, std::string const& s) override
    {
    }

    void put_diag(core::e_chan_num num, std::size_t width, parser::kas_diag const& diag) override
    {
        if (num == core::EMIT_DATA) {
            auto& cvt = elf_output.cvt;
            put(num, cvt.zero, width);
        }
    }

    // section control
    void set_section(core::core_section const& s) override
    {
        es_data_p = &es_data::core2es(s);
    }
    
    std::size_t position() const override
    {
        return es_data_p->position();
    }

    auto& file()
    {
        return elf_output;
    }

private:
    elf_file elf_output;        // output elf_external
    es_data *es_data_p{};       // current section
};

}

#endif
