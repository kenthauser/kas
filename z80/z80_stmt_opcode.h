#ifndef KAS_Z80_Z80_STMT_OPCODE_H
#define KAS_Z80_Z80_STMT_OPCODE_H



#include "z80_arg.h"
//#include "z80_arg_size.h"

#include "z80_formats_type.h"
//#include "z80_insn_serialize.h"
#include "z80_insn_validate.h"
#include "target/tgt_insn_serialize.h"
//#include "z80_insn_impl.h"

#include "kas_core/opcode.h"
#include "kas_core/core_print.h"

namespace kas::z80::opc
{
using namespace kas::core::opc;
using args_t = decltype(stmt_z80::args);
using op_size_t = kas::core::opcode::op_size_t;

struct z80_stmt_opcode : opcode
{
    using base_t = z80_stmt_opcode;
    using MCODE_T = z80_opcode_t;
    using mcode_size_t = typename MCODE_T::mcode_size_t;
    using arg_t        = typename MCODE_T::arg_t;

    //
    // gen_insn:
    //
    // A "kitchen sink" method which takes all args
    //
    // Generate header & save args as is appropriate for derived opcode
    //

    using ARGS_T = decltype(stmt_z80::args);
    
    virtual core::opcode& gen_insn(
                 // results of "validate" 
                   z80_insn_t   const&        insn
                 , z80_insn_t::insn_bitset_t& ok
                 , z80_opcode_t const        *opcode_p
                 , ARGS_T&&                    args
                 // and kas_core boilerplate
                 , Inserter& di
                 , fixed_t& fixed
                 , op_size_t& insn_size
                 ) = 0;

protected:
    // create a "container" for deserialized args
    template <typename READER_T>
    struct serial_args
    {
        serial_args(READER_T& reader, MCODE_T const& mcode)
            : mcode(mcode)
        {
            std::tie(code_p, args, update_handle) = tgt::opc::tgt_read_args(reader, mcode);
        }
        
        // create an `iterator` to allow range-for to process sizes
        struct iter : std::iterator<std::forward_iterator_tag, arg_t>
        {
            iter(serial_args const& obj, bool make_begin = {}) 
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
                auto tst = obj.args[index].mode() == MODE_NONE ? -1 : 0;
                return tst != other.index;
            }
        
        private:
            serial_args const& obj;
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
};
}
#endif
