#ifndef KAS_Z80_Z80_STMT_OPCODE_H
#define KAS_Z80_Z80_STMT_OPCODE_H



#include "z80_arg_defn.h"
//#include "z80_arg_size.h"

#include "z80_formats_type.h"
#include "z80_insn_serialize.h"
#include "z80_insn_validate.h"
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
        serial_args(READER_T& reader, z80_opcode_fmt const& fmt, uint16_t *opcode_p)
            : fmt(fmt), opcode_p(opcode_p)
        {
            std::tie(args, update_handle) = z80_read_args(reader, fmt, opcode_p);
        }
        
        // create an `iterator` to allow range-for to process sizes
        struct iter
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
                auto tst = obj.args[index].mode == MODE_NONE ? -1 : 0;
                return tst != other.index;
            }
        
        private:
            serial_args const& obj;
            int                index;
        };

        auto begin() const { return iter(*this, true); }
        auto end()   const { return iter(*this);       }
            
        void update_mode(z80_arg_t& arg)
        {
            auto n = &arg - args;
            z80_arg_update(n, arg, update_handle, fmt, opcode_p);
        }
        
        z80_opcode_fmt const& fmt;
        uint16_t        *opcode_p;
        z80_arg_t      *args;
        void            *update_handle;
    };
};
}
#endif
