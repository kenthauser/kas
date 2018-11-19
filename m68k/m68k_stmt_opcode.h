#ifndef KAS_M68K_M68K_STMT_OPCODE_H
#define KAS_M68K_M68K_STMT_OPCODE_H



#include "m68k_arg_defn.h"
#include "m68k_arg_size.h"

#include "m68k_formats_type.h"
#include "m68k_insn_serialize.h"
#include "m68k_insn_validate.h"
//#include "m68k_insn_impl.h"

#include "kas_core/opcode.h"
#include "kas_core/core_print.h"

namespace kas::m68k::opc
{
using namespace kas::core::opc;
using args_t = decltype(stmt_m68k::args);
using op_size_t = kas::core::opcode::op_size_t;

struct m68k_stmt_opcode : opcode
{
    using base_t = m68k_stmt_opcode;


    //
    // gen_insn:
    //
    // A "kitchen sink" method which takes all args
    //
    // Generate header & save args as is appropriate for derived opcode
    //

    using ARGS_T = decltype(stmt_m68k::args);
    
    virtual core::opcode& gen_insn(
                 // results of "validate" 
                   m68k_insn_t   const&        insn
                 , m68k_insn_t::insn_bitset_t& ok
                 , m68k_opcode_t const        *opcode_p
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
        serial_args(READER_T& reader, m68k_opcode_fmt const& fmt, uint16_t *opcode_p)
            : fmt(fmt), opcode_p(opcode_p)
        {
            std::tie(args, update_handle) = m68k_read_args(reader, fmt, opcode_p);
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
            
        void update_mode(m68k_arg_t& arg)
        {
            auto n = &arg - args;
            m68k_arg_update(n, arg, update_handle, fmt, opcode_p);
        }
        
        m68k_opcode_fmt const& fmt;
        uint16_t        *opcode_p;
        m68k_arg_t      *args;
        void            *update_handle;
    };
};
}
#endif
