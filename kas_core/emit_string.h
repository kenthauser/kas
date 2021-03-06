#ifndef KAS_CORE_EMIT_STRING_H
#define KAS_CORE_EMIT_STRING_H

#include "core_emit.h"
#include "core_section.h"
#include "emit_stream.h"
#include "expr/expr_fits.h"

// XXX is this out of order??
#include "kbfd/kbfd_target_reloc.h"


namespace kas::core
{
struct emit_formatted : emit_stream
{
    using emit_stream::emit_stream;
    virtual void do_put(e_chan_num, std::string const&) = 0;
    virtual void do_put_diag(e_chan_num, uint8_t, parser::kas_diag_t const&) = 0;
    virtual void do_put_reloc(e_chan_num, uint8_t, std::string const&) = 0;

    // configure hex formatter
    static constexpr int DIGITS_PER_BLOCK       = 4;
    static constexpr int DIGIT_BLOCK_SEPERATOR  = '_';
    static constexpr int DIGITS_PER_ADDR        = 6;

    std::string fmt_hex(uint8_t       bytes
                      , unsigned long value
                      , const char   *suffix = {}
                      ) const
    {
        // format value as hex string. append (reasonable length) suffix.
        static constexpr int MAX_SUFFIX_LENGTH = 10;
        char buf[sizeof(value) * 3 + MAX_SUFFIX_LENGTH + 1];
        auto *p = std::end(buf);
        *--p = 0;   // null terminate buffer

        if (suffix)
        {
            p -= std::strlen(suffix);
            std::strcpy(p, suffix);
        }

        auto digits = bytes * 2;     // two hex digits per byte

        for (int i = DIGITS_PER_BLOCK; digits--; --i)
        {
            if (!i && DIGITS_PER_BLOCK)
            {
                *--p = DIGIT_BLOCK_SEPERATOR;
                i = DIGITS_PER_BLOCK;
            }
            *--p = "0123456789abcdef"[value & 0xf];
            value >>= 4;
        }
        return p;   // NB: not dangling -- ctor for std::string
    }

    std::string fmt_diag(unsigned bytes) const
    {
        char out[sizeof(long long) * 3];
        auto p = std::end(out);
        *--p = 0;                   // zero terminate
        auto digits = bytes * 2;    // two hex digits per byte

        for (int i = DIGITS_PER_BLOCK; digits--; --i)
        {
            if (!i && DIGITS_PER_BLOCK)
            {
                *--p = DIGIT_BLOCK_SEPERATOR;
                i = DIGITS_PER_BLOCK;
            }
            *--p = 'x';
        }
        return p;   // NB: not dangling -- ctor for std::string
    }

    template <typename T>
    std::string fmt_addr(core_section const& section
                       , T const& offset
                       , long delta = {}) const
    {
        auto suffix = section_suffix(section);
        static constexpr auto ADDR_DIGITS = 
            std::min(sizeof(expression::e_addr_t) * 2, (size_t)DIGITS_PER_ADDR * 2);
        return fmt_hex((ADDR_DIGITS+1)/2, offset() + delta, suffix);
    }

    template <typename T>
    std::string fmt_addr(core_segment const& segment
                       , T const& offset
                       , long delta = {}) const
    {
        // XXX check if need to rebase `offset`
        return fmt_addr(segment.section(), offset, delta);
    }
    
    std::string fmt_addr(core_expr_dot const& dot) const
    {
        return fmt_addr(dot.segment(), dot.offset());
    }

    std::string fmt_addr(core_addr_t const& addr, long delta = 0) const
    {
        return fmt_addr(addr.section(), addr.offset(), delta);
    }

    void put_uint (e_chan_num num, uint8_t width, emit_value_t data) override
    {
        const char *sfx = suffix_p ? suffix_p : "ABS";
        do_put(num, fmt_hex(width, data, suffix_p));
        if (num == EMIT_DATA)
            *position_p += width;

        suffix_p = {};
    }

    template <typename T>
    void do_put_data(e_chan_num num, void const *v, unsigned num_chunks)
    {
        auto p = static_cast<T const *>(v);
        while (num_chunks--)
            do_put(num, fmt_hex(sizeof(T), *p++));
    }

    void put_raw(e_chan_num num, void const *p, uint8_t chunk_size, unsigned num_chunks) override
    {
        switch (chunk_size) 
        {
        case 1:
            do_put_data<uint8_t>(num, p, num_chunks);
            break;
        case 2:
            do_put_data<uint16_t>(num, p, num_chunks);
            break;
        case 4:
            do_put_data<uint32_t>(num, p, num_chunks);
            break;
        case 8:
            do_put_data<uint64_t>(num, p, num_chunks);
            break;
        default:
            throw std::logic_error(__FUNCTION__);
        }
        *position_p += chunk_size * num_chunks;
    }

    auto reloc_msg(kbfd::kbfd_target_reloc const& info
                 , std::string const& base_name
                 , int64_t addend
                 ) const -> std::string
    {
        // limit to 6 digits
        auto addr_bytes = 6;    // XXX
        if (addr_bytes > 3)
            addr_bytes = 3;

        auto width = info.reloc.bytes();

        std::ostringstream s;
        auto sfx = emit_formatted::section_suffix(*section_p);
        s << fmt_hex(addr_bytes, position(), sfx);
        s << " " << info.name;
        if (!base_name.empty())
            s << " " << base_name;

        // if addend fits in `width` bytes, show in value, not as rela
        using expression::expr_fits;

        // if width == 0, implies CHAN != DATA & just a display reloc...
        if (addend && width && 
                expr_fits().fits_sz(addend, width) != expression::DOES_FIT)
            s << " " << fmt_hex(width, addend);

        return s.str();
    }

    void put_section_reloc(
              e_chan_num num
            , kbfd::kbfd_target_reloc const& info
            , core_section const *section_p
            , int64_t  addend
            , bool use_rela
            ) override
    {
        // mark relocated data with `*` suffix
        suffix_p = "*";

        auto msg = reloc_msg(info, section_p ? section_p->name() : "", addend);
        do_put_reloc(num, 0, msg);  // just use zero for width
    }

    void put_symbol_reloc(
              e_chan_num num
            , kbfd::kbfd_target_reloc const& info
            , core_symbol_t const& sym
            , int64_t addend 
            , bool use_rela
            ) override
    {
        // mark relocated data with `*` suffix
        suffix_p = "*";

        auto msg = reloc_msg(info, sym.name(), addend);
        do_put_reloc(num, 0, msg);  // just use zero for width
    }


    void put_diag(e_chan_num num, uint8_t width, parser::kas_diag_t const& diag) override
    {
        do_put_diag(num, width, diag);
        if (num == EMIT_DATA)
            *position_p += width;
        suffix_p = {};
    }

    void set_section(core_section const& section) override
    {
        section_p  = &section;
        position_p = &position_map[section.index()];
    }

    std::size_t position() const override
    {
        return *position_p;
    }

    static const char *section_suffix(core_section const& section)
    {
        // ensure .text/.data/.bss are sections 1/2/3 for traditional output
        // see ctor for `core::assemble`
        static constexpr const char * suffixes[] =
            {"\'", "\"", "%", "x", "y", "z", "@"};

        static constexpr uint32_t num_suffixes =
                    std::end(suffixes) - std::begin(suffixes);

        auto section_n = section.index() - 1;
        return section_n < num_suffixes ? suffixes[section_n] : "+";
    }
    
    // instance variables
    std::map<typename core_section::index_t, std::size_t> position_map;
    const core_section *section_p{};
    std::size_t *position_p{};
    const char *suffix_p {};
};


//
// Minimal concrete class using ABC `emit_string`
//

struct emit_raw : emit_formatted
{
    // put ascii format to std::string
    using emit_formatted::emit_formatted;

    void do_put(e_chan_num chan, std::string const& s) override
    {
        const char *pfx;
        switch (chan)
        {
            case EMIT_DATA:
                pfx = "";
                break;
            case EMIT_ADDR:
                pfx = "A:";
                break;
            case EMIT_EXPR:
                pfx = "E:";
                break;
            case EMIT_INFO:
                pfx = "I:";
                break;
            default:
                throw std::runtime_error("emit_raw::do_put: chan unimplemented");
        }

        // combine bits before emitting to output stream
        out << (pfx + s + " ");
    }

    void do_put_diag(e_chan_num num, uint8_t width, parser::kas_diag_t const& diag)
            override
    {
        out << "[Err: " + diag.message + "](" << +width << " bytes) ";
    }

    void do_put_reloc(e_chan_num num, uint8_t width, std::string const& msg)
            override
    {
        out << "[Reloc: " + msg + "] ";
    }
};
}





#endif
