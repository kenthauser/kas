#ifndef KAS_TARGET_TGT_OPC_QUICK_H
#define KAS_TARGET_TGT_OPC_QUICK_H

// Special opcode for the extremely common case of 
//
// 1. All args are constant (registers or fixed point)
// 2. Entire machine-code fits in `fixed` area of opcode
// 3. All "emits" are sizeof `mcode_size_t`
//
// Note that this `opcode` is instruction-set agnostic. Only the
// `machine-code` size is variable. 

// Implementation notes:
//
// A "new" emit-subclass, `emit_quick` is created. This "emitter" accepts
// fixed writes of size sizeof `mcode_size_t`. All others it absorbs & 
// sets error flag. At end of "opcode.emit", the error flag is tested to see
// if "quick" requirements are satisified.

#include "tgt_data_inserter.h"
#include "kas_core/opcode.h"
#include "kas_core/emit_stream.h"
#include "kas_core/core_insn.h"


namespace kas::tgt::opc
{

namespace detail
{
    // allow 
    template <typename mcode_size_t, typename Inserter>
    struct quick_stream : core::emit_stream
    {
        using e_chan_num = core::e_chan_num;
        quick_stream(Inserter& inserter, unsigned size) : di(inserter), size(size) {}

        void put_uint(e_chan_num num, std::size_t width, uint64_t data) override
        {
            // if mcode_t insert, perform
            if (width == sizeof(mcode_size_t) && ok && size)
            {
                di(data, width);
                size -= width;
            }

            // wrong size or exceeds max size
            else
                set_error(num);

        }
        void put_data(e_chan_num num, void const *, std::size_t, std::size_t) override
        {
            set_error(num);        // always error
        }
        
        // emit reloc
        void put_symbol_reloc(e_chan_num num, uint32_t, core::core_symbol const&, int64_t&) 
                override
        {
            set_error(num);
        }
        
        void put_section_reloc(e_chan_num num, uint32_t, core::core_section const&, int64_t&)
                override
        {
            set_error(num);
        }

        // emit diagnostics
        void put_diag(e_chan_num num, std::size_t, parser::kas_diag const&) override
        {
            set_error(num);
        };

        // XXX emit temp diagnostic message (type not known)
        void put_str(e_chan_num num, std::size_t, std::string const&) override
        {
            set_error(num);
        }
        
        // current section interface
        void set_section(core::core_section const&) override
        {
        };
        std::size_t position() const override
        {
            return 0;       // throw???
        }

        operator bool() const
        {
            return ok;
        }

    private:
        void set_error(e_chan_num num)
        {
            if (num == core::EMIT_DATA)
                ok = false;
        }

        Inserter& di;
        unsigned  size;
        bool      ok{true};
    };
        
    template <typename mcode_size_t, typename Inserter>
    struct quick_emit : core::emit_base
    {
        quick_emit(Inserter& inserter, unsigned size)
            : stream(inserter, size)
            , emit_base{stream}
            {}
        
        void emit(core::core_insn& insn, core::core_expr_dot const *dot_p) override
        {
            // dot unknown 
            insn.emit(*this, nullptr);
        }

        quick_stream<mcode_size_t, Inserter> stream;
    };
}




template <typename mcode_size_t>
struct tgt_opc_quick : core::opc::opcode
{
    OPC_INDEX();

    const char *name() const override
    {
        return "TGT_Q";
    }

    template <typename Inserter, typename mcode, typename ARGS>
    bool proc_args(Inserter& di, mcode const& op, ARGS& args, unsigned size)
    {
        std::cout << "TGT_QUICK::proc_args()";
        auto inserter = tgt_data_inserter<mcode_size_t, Inserter>(di, *fixed_p);
        auto emitter  = detail::quick_emit<mcode_size_t, decltype(inserter)>(inserter, size);

        uint16_t code[2];
        if (op.opc_long)
        {
            code[0] = op.code() >> 8;
            code[1] = op.code();
        } else
        {
            code[0] = op.code();
        }

        auto& fmt  = op.fmt();
        auto& vals = op.vals();
        auto val_iter     = vals.begin();
        auto val_iter_end = vals.end();

        // always validator for each arg
        unsigned n = 0;
        for (auto& arg : args)
            fmt.insert(n++, code, arg, &*val_iter++);

        std::cout << " before -> " << std::boolalpha << (bool)emitter.stream;

        op.emit(emitter, code, args);
        std::cout << " -> " << std::boolalpha << (bool)emitter.stream << std::endl;
        return emitter.stream;
    }
    
    
    void fmt(Iter it, uint16_t cnt, std::ostream& os) override
    {
        auto words = (*size_p)() / sizeof(mcode_size_t);
        auto p     = fixed_p->begin<mcode_size_t>();

        os << std::hex << ":";
        while (words--)
            os << " " << +*p++;
    }

    void emit(Iter it, uint16_t cnt, core::emit_base& base, core::core_expr_dot const *dot_p) override
    {
        auto words = (*size_p)() / sizeof(mcode_size_t);
        auto p     = fixed_p->begin<mcode_size_t>();
        while (words--)
            base << *p++;
    }
};

}

#endif

