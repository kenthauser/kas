#ifndef KAS_M68K_OPC_GENERAL_H
#define KAS_M68K_OPC_GENERAL_H


#include "m68k_stmt_opcode.h"
#include "m68k_opcode_emit.h"

namespace kas::m68k::opc
{
using namespace kas::core::opc;
using args_t = decltype(stmt_m68k::args);
using op_size_t = kas::core::opcode::op_size_t;

struct m68k_opc_general : m68k_stmt_opcode
{
    using base_t::base_t;

    OPC_INDEX();

    const char *name() const override { return "M68K_GEN"; }
    
    core::opcode& gen_insn(
                 // results of "validate" 
                   m68k_insn_t   const&        insn
                 , m68k_insn_t::insn_bitset_t& ok
                 , m68k_opcode_t const        *opcode_p
                 , ARGS_T&&                    args
                 // and kas_core boilerplate
                 , Inserter& di
                 , fixed_t& fixed
                 , op_size_t& insn_size
                 ) override
    {
        // get size for this opcode
        auto& op = *opcode_p;
       
        insn_size = sizeof(uint16_t) * (1  + op.opc_long);
        for (auto& arg : args)
            insn_size += arg.size(expression::expr_fits{});
        
        // serialize format (for resolved instructions)
        // 1) opcode index
        // 2) opcode binary data (word or long)
        // 3) serialized args

        auto inserter = m68k_data_inserter(di, fixed);
        
        inserter(op.index, M_SIZE_WORD);
        auto data_p = inserter(op.code(), op.opc_long ? M_SIZE_LONG : M_SIZE_WORD);
        m68k_insert_args(inserter, std::move(args), data_p, op.fmt());
        return *this;
    }
    
    void fmt(Iter it, uint16_t cnt, std::ostream& os) override
    {
        // deserialize insn data
        // format:
        //  1) opcode index
        //  2) opcode binary code (word or long)
        //  3) serialized args
        auto  reader = m68k_data_reader(it, *fixed_p, cnt);
        auto& opcode = m68k_opcode_t::get(reader.get_fixed(M_SIZE_WORD));
        auto  op_p   = reader.get_fixed_p(opcode.opc_long ? M_SIZE_LONG : M_SIZE_WORD);
        auto  args   = serial_args(reader, opcode.fmt(), op_p);

        // print "name" & "size"...
        os << opcode.defn().name() << ":";
        os << m68k_insn_size::m68k_size_suffixes[opcode.insn_sz]; 
        
        // ...print opcode...
        os << std::hex << " " << std::setw(2) << *op_p++;
        if (opcode.opc_long)
            os << "'" << std::setw(2) << *op_p;

        // ...and args
        auto delim = " : ";
        for (auto& arg : args)
        {
            os << delim << arg;
            delim = ",";
        }
    }

    op_size_t calc_size(Iter it, uint16_t cnt, core::core_fits const& fits) override
    {
        // deserialize insn data
        // format:
        //  1) opcode index
        //  2) opcode binary code (word or long)
        //  3) serialized args
        auto  reader = m68k_data_reader(it, *fixed_p, cnt);
        auto& opcode = m68k_opcode_t::get(reader.get_fixed(M_SIZE_WORD));
        auto  op_p   = reader.get_fixed_p(opcode.opc_long ? M_SIZE_LONG : M_SIZE_WORD);
        auto  args   = serial_args{reader, opcode.fmt(), op_p};

        // base instruction size
        op_size_t new_size = sizeof(uint16_t) * (1 + opcode.opc_long);
       
        // evaluate args with new `fits`
        for (auto& arg : args)
        {
            auto old_mode = arg.mode;
            new_size += arg.size(fits);
            if (arg.mode != old_mode)
                args.update_mode(arg);
        }
        
        return new_size;
    }

    void emit(Iter it, uint16_t cnt, core::emit_base& base, core::core_expr_dot const& dot) override
    {
#if 1
        // deserialze insn data
        // format:
        //  1) opcode index
        //  2) opcode binary code (word or long)
        //  3) serialized args
        auto  reader = m68k_data_reader(it, *fixed_p, cnt);
        auto& opcode = m68k_opcode_t::get(reader.get_fixed(M_SIZE_WORD));
        auto  op_p   = reader.get_fixed_p(opcode.opc_long ? M_SIZE_LONG : M_SIZE_WORD);
        auto  args   = serial_args{reader, opcode.fmt(), op_p};

        opcode.emit(base, op_p, args, dot);
#endif
    }
};
}
#endif
