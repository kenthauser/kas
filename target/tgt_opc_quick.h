#ifndef KAS_TARGET_TGT_OPC_QUICK_H
#define KAS_TARGET_TGT_OPC_QUICK_H

// Special opcode for the extremely common case of 
//
// All args are constant (registers or fixed point)
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
    // allow all 
    template <typename mcode_size_t, typename Inserter>
    struct quick_stream : core::emit_stream
    {
        using e_chan_num = core::e_chan_num;
        quick_stream(Inserter& inserter, unsigned size) : di(inserter), size(size) {}

        void put_uint(e_chan_num num, std::size_t width, uint64_t data) override
        {
        #if 1
            di(data, width);
        #else
            // if mcode_t insert, perform
            if (width == sizeof(mcode_size_t) && ok && size)
            {
                di(data, width);
                size -= width;
            }

            // wrong size or exceeds max size
            else
                set_error(num);
        #endif

        }
        void put_data(e_chan_num num, void const *, std::size_t, std::size_t) override
        {
            set_error(__FUNCTION__);        // always error
        }
        
        // emit reloc
        void put_symbol_reloc(
                  e_chan_num num
                , core::reloc_info_t const& info
                , uint8_t width
                , uint8_t offset
                , core::core_symbol const& sym
                , int64_t addend
                ) override
        {
            set_error(__FUNCTION__);
        }
        
        void put_section_reloc(
                  e_chan_num num
                , core::reloc_info_t const& info
                , uint8_t width
                , uint8_t offset
                , core::core_section const& section
                , int64_t addend
                ) override
        {
            set_error(__FUNCTION__);
        }

        // emit diagnostics
        void put_diag(e_chan_num num, std::size_t, parser::kas_diag const&) override
        {
            set_error(__FUNCTION__);
        };

        // XXX emit temp diagnostic message (type not known)
        void put_str(e_chan_num num, std::size_t, std::string const&) override
        {
            set_error(__FUNCTION__);
        }
        
        // current section interface
        void set_section(core::core_section const&) override
        {
            set_error(__FUNCTION__);
        };

        std::size_t position() const override
        {
            set_error(__FUNCTION__);
            return {};
        }

    private:
        void set_error(const char *fn) const
        {
            std::string msg{fn};
            throw std::logic_error("quick_stream: " + msg + " unimplemented");
        }

        
        Inserter& di;
        unsigned  size;
    };
       
    // use helper function to perform partial specialization
    template <typename mcode_size_t, typename Inserter>
    struct quick_emit_t : core::emit_base
    {
        quick_emit_t(Inserter& inserter, unsigned size)
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

    // c++17 doesn't allow partial deduction guides. Sigh. 
    template <typename mcode_size_t, typename Inserter>
    auto quick_emit(Inserter& inserter, unsigned size)
    {
        return quick_emit_t<mcode_size_t, Inserter>(inserter, size);
    }
}


// just a "standard" opcode
template <typename MCODE_T>
struct tgt_opc_quick : core::opcode
{
    using mcode_size_t = typename MCODE_T::mcode_size_t;
    using stmt_info_t  = typename MCODE_T::stmt_info_t;

    OPC_INDEX();

    using NAME = str_cat<typename MCODE_T::BASE_NAME, KAS_STRING("_QUICK")>;
    const char *name() const override { return NAME::value; }
   
    template <typename ARGS>
    void proc_args(core::opcode::data_t& data
                 , MCODE_T const& mcode
                 , ARGS& args
                 , stmt_info_t const& info)
    {
        // NB: data.size already set
        auto inserter = tgt_data_inserter_t<MCODE_T>(data);
        auto emitter  = detail::quick_emit<mcode_size_t>(inserter, data.size());
        mcode.emit(emitter, std::move(args), info);
    }
    
    
    void fmt(data_t const& data, std::ostream& os) const override
    {
        auto words = data.size() / sizeof(mcode_size_t);
        auto p     = data.fixed.begin<mcode_size_t>();

        os << std::hex;
        while (words--)
        {
            os << +*p++;
            if (words)
                os << " ";
        }
    }

    void emit(data_t const& data, core::emit_base& base, core::core_expr_dot const *dot_p) const override
    {
        auto words = data.size() / sizeof(mcode_size_t);
        auto p     = data.fixed.begin<mcode_size_t>();
        while (words--)
            base << *p++;
    }
};

}

#endif

