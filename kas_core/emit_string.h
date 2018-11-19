#ifndef KAS_CORE_EMIT_STRING_H
#define KAS_CORE_EMIT_STRING_H

#include "core_emit.h"
#include "core_section.h"


namespace kas::core
{
    struct emit_formatted : emit_stream
    {
        std::function<void(e_chan_num, std::string const&)> put;
        std::function<void(e_chan_num, std::size_t, parser::kas_diag const&)> emit_diag;
        std::map<unsigned, std::size_t> position_map;
        const core_section *section_p{};
        std::size_t  *position_p{};
        //int64_t  offset {};
        const char *suffix_p {};

        emit_formatted(decltype(put) put_, decltype(emit_diag) diag_) :
                put(put_), emit_diag(diag_)
        {
            // XXX: should be in `core_emit`, but test fixture include issue.
            set_section(core_section::get(".text"));
        }

        std::string fmt_hex(std::size_t n, unsigned long value, const char *suffix = nullptr) const
        {
            // format value as hex string. append (reasonable length) suffix.
            static constexpr int MAX_SUFFIX_LENGTH = 10;
            char buf[sizeof(value) * 3 + MAX_SUFFIX_LENGTH + 1];
            auto *p = std::end(buf);
            *--p = 0;   // null terminate buffer

            if (suffix) {
                p -= std::strlen(suffix);
                std::strcpy(p, suffix);
            }

            n *= 2;     // two hex digits per byte

            static constexpr int DIGITS_PER_BLOCK = 4;
            static constexpr int DIGIT_BLOCK_SEPERATOR = '_';
            for (int i = DIGITS_PER_BLOCK; n--; i--) {
                if (!i && DIGITS_PER_BLOCK) {
                    *--p = DIGIT_BLOCK_SEPERATOR;
                    i = DIGITS_PER_BLOCK;
                }
                *--p = "0123456789abcdef"[value & 0xf];
                value >>= 4;
            }
            return p;   // NB: not dangling -- ctor for std::string
        }

        std::string fmt_addr(std::size_t n, core_expr_dot const& dot) const
        {
            auto offset = dot.offset()();
            auto suffix = section_suffix(dot.section());
            return fmt_hex(n, offset, suffix);
        }

        std::string fmt_addr(std::size_t n, core_addr const& addr, unsigned long delta = 0) const
        {
            auto offset = addr.offset();
            auto suffix = section_suffix(addr.section());
            return fmt_hex(n, offset() + delta, suffix);
        }

        void put_uint (e_chan_num num, std::size_t width, uint64_t data) override
        {
            put(num, fmt_hex(width, data, suffix_p));
            if (num == EMIT_DATA)
                *position_p += width;

            suffix_p = {};
        }

        template <typename T>
        void do_put_data(e_chan_num num, void const *v, std::size_t num_chunks)
        {
            auto p = static_cast<T const *>(v);
            while (num_chunks--)
                put(num, fmt_hex(sizeof(T), *p++));
        }

        void put_data (e_chan_num num, void const *p, std::size_t chunk_size, std::size_t num_chunks) override
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

        void put_section_reloc(
                  e_chan_num num
                , uint32_t    reloc
                , core_section const& section
                , int64_t& data
                ) override
        {

            // offsets are emitted via `put_int`.
            if (!suffix_p)
                suffix_p = emit_formatted::section_suffix(section);
            else
                suffix_p = "?";
        }

        void put_symbol_reloc(
                  e_chan_num num
                , uint32_t    reloc
                , core_symbol const& sym
                , int64_t& data
                ) override
        {
            // only external symbols resolve here
            if (!suffix_p)
                suffix_p = "*";
            else
                suffix_p = "X";
        }

        void put_str(e_chan_num num, std::size_t, std::string const& s) override
        {
            put(num, s);
        }

        void put_diag(e_chan_num num, std::size_t width, parser::kas_diag const& diag) override
        {
            emit_diag(num, width, diag);
            if (num == EMIT_DATA)
                *position_p += width;
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

    };

    struct emit_str : emit_base
    {
        // put ascii format to std::string
        emit_str() : emit_base(fmt) {}

        // interface functions
        std::string const& out() const
        {
            return buffer;
        }

        void clear()
        {
            buffer.clear();
        }

    private:
        // implementation

        decltype(emit_formatted::put) put(std::string &out)
        {
            return [&](e_chan_num num, std::string const& s)
            {
                switch (num)
                {
                    case EMIT_DATA:
                        break;
                    case EMIT_ADDR:
                        out += "A:";
                        break;
                    case EMIT_EXPR:
                        out += "E:";
                        break;
                    case EMIT_INFO:
                        out += "I:";
                        break;
                    default:
                        throw std::runtime_error("emit_str::num");
                }
                out += s + " ";
            };
        }

        decltype(emit_formatted::emit_diag) emit_diag(std::string &out)
        {
            return [&](e_chan_num num, std::size_t width, parser::kas_diag const& diag)
            {
                out += "[Err: " + diag.message + "] ";
            };
        }

        emit_formatted fmt{put(buffer), emit_diag(buffer)};
        std::string buffer;
    };

}





#endif
