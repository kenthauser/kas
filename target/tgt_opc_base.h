#ifndef KAS_TARGET_TGT_OPC_BASE_H
#define KAS_TARGET_TGT_OPC_BASE_H


#include "tgt_insn_serialize.h"

#include "kas_core/opcode.h"
#include "kas_core/core_print.h"

namespace kas::tgt::opc
{

template <typename MCODE_T>
struct tgt_opc_base : core::opc::opcode
{
    using base_t       = tgt_opc_base;
    using mcode_t      = MCODE_T;
    
    using insn_t       = typename mcode_t::insn_t;
    using bitset_t     = typename insn_t::bitset_t;
    using arg_t        = typename mcode_t::arg_t;
    using arg_mode_t   = typename mcode_t::arg_mode_t;
    using stmt_info_t  = typename mcode_t::stmt_info_t;
    using stmt_args_t  = typename mcode_t::stmt_args_t;
    using mcode_size_t = typename mcode_t::mcode_size_t;

    using op_size_t    = typename core::opcode::op_size_t;
    using emit_value_t = typename core::core_emit::emit_value_t;
   
    //
    // gen_insn:
    //
    // A "kitchen sink" method which takes all args
    //
    // Generate header & save args as is appropriate for derived opcode
    //

    virtual core::opcode *gen_insn(
                 // results of "validate" 
                   insn_t const&  insn
                 , bitset_t&      ok
                 , mcode_t const& mcode
                 , stmt_args_t&&  args
                 , stmt_info_t    stmt_info

                 // and kas_core boilerplate
                 , opcode::data_t& data
                 ) = 0;

protected:
    static auto tgt_data_inserter(data_t& data)
    {
        return tgt_data_inserter_t<mcode_size_t, emit_value_t>(data);
    }

    static auto tgt_data_reader(data_t const& data)
    {
        return tgt_data_reader_t<mcode_size_t, emit_value_t>(data);
    }

    // create a "container" for deserialized args
    template <typename READER_T>
    struct serial_args_t
    {
        serial_args_t(READER_T& reader, MCODE_T const& mcode)
            : mcode(mcode)
        {
            std::tie(code_p, args) = tgt::opc::tgt_read_args(reader, mcode);
            info = mcode.extract_info(code_p);
        }
        
        // create an `iterator` to allow range-for to process sizes
        struct iter : std::iterator<std::forward_iterator_tag, arg_t>
        {
            iter(serial_args_t const& obj, bool make_begin = {}) 
                    : obj(obj)
                    , index(make_begin ? 0 : -1)
                    {}

            // range operations
            auto& operator++() 
            {
                ++index;
                return *this;
            }
            auto& operator*() const
            { 
                return obj.args[index];
            }
            auto operator!=(iter const& other) const
            {
                auto tst = obj.args[index].mode() == arg_t::MODE_NONE ? -1 : 0;
                return tst != other.index;
            }
        
        private:
            serial_args_t const& obj;
            int                index;
        };

        auto begin() const { return iter(*this, true); }
        auto end()   const { return iter(*this);       }
       
        // instance variables
        MCODE_T const& mcode;
        mcode_size_t  *code_p;
        arg_t         *args;
        stmt_info_t    info;
        void          *wb_handle;       // opaque writeback handle
    };
    
    template <typename READER_T>
    static auto serial_args(READER_T& reader, MCODE_T const& mcode)
    {
        return serial_args_t<READER_T>(reader, mcode); 
    }
};
}
#endif
