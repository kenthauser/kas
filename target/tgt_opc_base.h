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
    using stmt_args_t  = typename mcode_t::stmt_args_t;
    using mcode_size_t = typename mcode_t::mcode_size_t;

    using op_size_t    = typename core::opcode::op_size_t;
   

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
                 , mcode_t const *mcode_p
                 , stmt_args_t&&  args

                 // and kas_core boilerplate
                 , opcode::data_t& data
                 ) = 0;

protected:
    static auto tgt_data_inserter(data_t& data)
    {
        return tgt_data_inserter_t<MCODE_T>(data);
    }

    static auto tgt_data_reader(data_t const& data)
    {
        return tgt_data_reader_t<MCODE_T>(data);
    }

    // create a "container" for deserialized args
    template <typename READER_T>
    struct serial_args_t
    {
        serial_args_t(READER_T& reader, MCODE_T const& mcode)
            : mcode(mcode)
        {
            std::tie(code_p, args, update_handle) = tgt::opc::tgt_read_args(reader, mcode);
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
            
        void update_mode(arg_t& arg)
        {
            auto n = &arg - args;
            tgt_arg_update(mcode, n, arg, update_handle, code_p);
        }
        
        MCODE_T const& mcode;
        mcode_size_t  *code_p;
        arg_t         *args;
        void          *update_handle;
    };

    template <typename READER_T>
    static auto serial_args(READER_T& reader, MCODE_T const& mcode)
    {
        return serial_args_t<READER_T>(reader, mcode); 
    }
};
}
#endif