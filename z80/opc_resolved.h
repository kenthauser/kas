#ifndef KAS_Z80_Z80_OPC_RESOLVED_H
#define KAS_Z80_Z80_OPC_RESOLVED_H

#if 0

define insn_emit or some-such

insn takes all "core_emit" methods (as stream) & encapsulates as insn


//
// OPC


q




#endif
//
// Accumulate register-pair args in "zero" base-code 32-bit word.
// Emit opcode base code & register data
//

#include "z80_stmt_opcode.h"
#include "z80_opcode_emit.h"
#include "utility/align_as_t.h"

namespace kas::z80::opc
{

template <typename Inserter>
struct opc_stream : core::emit_stream
{
    enum { R_NONE, R_BYTE, R_WORD, R_LONG, R_QUAD, R_DIAG };
    using value_type = typename Inserter::value_type;
    
    struct stream_info : kas::detail::alignas_t<stream_info, value_type>
    {
        struct value_type;
        std::array<uint8_t, sizeof(value_type)> info;
    };

    opc_stream(Inserter& inserter) : inserter(inserter) {}


    void put_uint(core::e_chan_num num, std::size_t width, uint64_t data) override
    {
        do_put_data(width, data);
    }

    void put_data(core::e_chan_num num, void const *p, std::size_t chunk_size, std::size_t num_chunks) override
    {
        auto get_one_chunk = [p=static_cast<const char *>(p), &chunk_size]() mutable -> uint64_t
            {
                auto old_p = p;     // perform manual `*p++`
                p += chunk_size;
                switch (chunk_size)
                {
                    case 1: return *reinterpret_cast<uint8_t  const *>(old_p);
                    case 2: return *reinterpret_cast<uint16_t const *>(old_p);
                    case 4: return *reinterpret_cast<uint32_t const *>(old_p);
                    case 8: return *reinterpret_cast<uint64_t const *>(old_p);
                    default: return 0;
                }
            };

        while(num_chunks--)
            do_put_data(chunk_size, get_one_chunk());
    };
    
    
    template <typename Reader>
    void fmt(Reader& reader, std::ostream& os) const
    {
        while (!reader.empty())
        {
            auto info = get_info(reader);
            int width;
            switch(info)
            {
                case R_DIAG:
                    width = reader.get_fixed(M_SIZE_WORD);
                    os << std::hex << std::setw(width) << reader.get_expr();
                    continue;
                
                case R_BYTE: width = 1; break;
                case R_WORD: width = 2; break;
                case R_LONG: width = 4; break;
                case R_QUAD: width = 8; break;

                default:
                    throw std::logic_error{"opc_stream::emit: switch(info)"};
            }

            os << std::hex << std::setw(width);
            
            uint64_t data{};
            while (width > 0)
            {
                data <<= 16;
                data |= reader.get_fixed(M_SIZE_WORD);
            }
            os << data;
        }
    }

    
    template <typename Reader>
    void emit(Reader& reader, core::emit_base& base) const
    {
        while (!reader.empty())
        {
            auto info = get_info(reader);
            int width;
            switch(info)
            {
                case R_BYTE: width = 1; break;
                case R_WORD: width = 2; break;
                case R_LONG: width = 4; break;
                case R_QUAD: width = 8; break;
                case R_DIAG:
                    width = reader.get_fixed(M_SIZE_WORD);
                    base << core::set_size(width) << reader.get_expr();
                    continue;
                default:
                    throw std::logic_error{"opc_stream::emit: switch(info)"};
            }

            base << core::set_size(width);
            
            uint64_t data{};
            while (width > 0)
            {
                data <<= 16;
                data |= reader.get_fixed(M_SIZE_WORD);
            }
            base << data;
        }
    }

    
    // emit diagnostics
    void put_diag(core::e_chan_num, std::size_t width, parser::kas_diag const& diag) override
    {
        do_put_diag(width, diag);
    };

    //
    // Methods not supported by "opc_resolved"
    //
    void put_symbol_reloc(
              core::e_chan_num num
            , uint32_t    reloc
            , core::core_symbol const& sym
            , int64_t& data
            ) override
        { unsupported("put_symbol_reloc"); }

    void put_section_reloc(
              core::e_chan_num num
            , uint32_t    reloc
            , core::core_section const& section
            , int64_t& data
            ) override
        { unsupported("put_section_reloc"); }

    // XXX emit temp diagnostic message (type not known)
    void put_str(core::e_chan_num, std::size_t, std::string const&) override
        { unsupported("put_str"); }
    
    // current section interface
    void set_section(core::core_section const&) override
        { unsupported("set_section"); }

    std::size_t position() const override
        { return 0; };

private:
    // put data in buffer as "words". strored little endian
    void do_put_data(int width, uint64_t data)
    {
        int info;

        switch (width)
        {
            case 1: info = R_BYTE; break;
            case 2: info = R_WORD; break;
            case 4: info = R_LONG; break;
            case 8: info = R_QUAD; break;
            default:
                throw std::logic_error{"opc_stream: invalid size"};
        }

        put_info(info);
        while (width > 0)
        {
            inserter(data, M_SIZE_WORD);
            width -= 2;
            data >>= 16;
        };
    }


    void do_put_diag(int width, parser::kas_diag const& diag)
    {
        put_info(R_DIAG);
        inserter(width, M_SIZE_WORD);
        inserter(diag, M_SIZE_AUTO);
    }

    template <typename Reader>
    auto& get_diag(Reader& reader)
    {
        return reader.get_expr();
    }

    void put_info(int info)
    {
        if (cnt == 0 || !info_p)
        {
            info_p = inserter(0, M_SIZE_WORD);
            cnt = sizeof(value_type);
        }

        info_p[--cnt] = info;
    }

    template <typename Reader>
    auto get_info(Reader& reader)
    {
        if (cnt == 0 || !info_p)
        {
            info_p = reader.get_fixed_p(M_SIZE_WORD);
            cnt = sizeof(value_type);
        }
        return info_p[--cnt];
    }


    void unsupported(const char *msg) const
    {
        throw std::logic_error{std::string("opc_stream: unsupported method: ") + msg};
    }


    stream_info *info_p {};
    int cnt             {};
    Inserter &inserter;
};


struct z80_opc_resolved : z80_stmt_opcode
{
    using base_t::base_t;
    
    OPC_INDEX();
    const char *name() const override { return "Z80_RESOLVED"; };

    struct opc_emit : core::core_base
    {
        void emit(core::core_insn& insn, core::core_expr_dot const& dot) override
        {
            insn.emit(*this, dot);
        }
    };


    core::opcode& gen_insn(
                 // results of "validate" 
                   z80_insn_t   const&        insn
                 , z80_insn_t::insn_bitset_t& ok
                 , z80_opcode_t const        *opcode_p
                 , ARGS_T&&                    args
                 // and kas_core boilerplate
                 , Inserter& di
                 , fixed_t& fixed
                 , op_size_t& insn_size
                 ) override
    {
        // get opcode & inserter
        auto& opcode  = *opcode_p;
        auto inserter = z80_data_inserter(di, fixed);
       
        // get opcode "base" value
        uint16_t code[2];
        if (opcode.opc_long)
        {
            code[0] = opcode.code() >> 16;
            code[1] = opcode.code();
        }
        else
        {
            code[0] = opcode.code();
        }

        // Insert args into opcode "base" value
        auto& fmt = opcode.fmt();
        unsigned n = 0;
        for (auto& arg : args)
            fmt.insert(n++, code, arg);

        // create a "core_emit" object
        auto emit_obj = opc_emit(opc_stream(inserter));


        // now use common emit
        z80_opcode_emit(emit_obj, opcode, code, args, core::core_expr_dot());
        
        return *this;
    }
    
    void fmt(Iter it, uint16_t cnt, std::ostream& os) override
    {
        auto  reader = z80_data_reader(it, *fixed_p, cnt);
        auto n = (*size_p)() / 2;

        os << std::hex;
        while (n-- > 0) 
            os << reader.get_fixed(M_SIZE_WORD) << " ";
    }

    void emit(Iter it, uint16_t cnt, core::emit_base& base, core::core_expr_dot const& dot) override
    {
        auto  reader = z80_data_reader(it, *fixed_p, cnt);
        auto n = (*size_p)() / 2;

        // emit as words
        while (n-- > 0) 
            base << (uint16_t)reader.get_fixed(M_SIZE_WORD);
    }
};
}

#endif
