#ifndef KAS_Z80_OPC_GENERAL_H
#define KAS_Z80_OPC_GENERAL_H


#include "z80_stmt_opcode.h"
#include "z80_opcode_emit.h"
#include "target/tgt_insn_serialize.h"

namespace kas::z80::opc
{
using namespace kas::core::opc;
using namespace kas::tgt::opc;

using args_t = decltype(stmt_z80::args);
using op_size_t = kas::core::opcode::op_size_t;

struct z80_opc_general : z80_stmt_opcode
{
    using base_t::base_t;
    //using MCODE_T      = z80_opcode_t;
    //using mcode_size_t = typename MCODE_T::mcode_size_t;
    //using arg_t        = typename MCODE_T::arg_t;

    OPC_INDEX();

    const char *name() const override { return "Z80_GEN"; }
    
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
        // get size for this opcode
        auto& op = *opcode_p;
        
        std::cout << "opc_general::gen_insn: " << insn.name();
        auto delim = ": ";
        for (auto& arg : args)
        {
            std::cout << delim << arg;
            delim = ", ";
        }
        std::cout << std::endl;
        
        op.size(args, insn_size, expression::expr_fits{}, trace);

        // serialize format (for resolved instructions)
        // 1) opcode index
        // 2) opcode binary data (word or long)
        // 3) serialized args
#if 0
        auto inserter = z80_data_inserter(di, fixed);
        
        inserter(op.index, M_SIZE_WORD);
        z80_insert_args(inserter, op, std::move(args));
#else
        auto inserter = tgt_data_inserter<mcode_size_t>(di, fixed);
        inserter(op.index);
        tgt_insert_args(inserter, op, std::move(args));

#endif
        return *this;
    }
    
    void fmt(Iter it, uint16_t cnt, std::ostream& os) override
    {
        // deserialize insn data
        // format:
        //  1) opcode index
        //  2) opcode binary code (word or long)
        //  3) serialized args
#if 0
        auto  reader = z80_data_reader(it, *fixed_p, cnt);
        auto& opcode = z80_opcode_t::get(reader.get_fixed(M_SIZE_WORD));
        auto  args   = serial_args(reader, opcode);
        // print "name" & "size"...
        os << opcode.defn().name();
        
        // ...print opcode...
        os << std::hex << " " << std::setw(2) << *args.data_p++;
        if (opcode.opc_long)
            os << "'" << std::setw(2) << *args.data_p;

        // ...and args
        auto delim = " : ";
        for (auto& arg : args)
        {
            os << delim << arg;
            delim = ",";
        }
#else

        auto  reader = tgt_data_reader<mcode_size_t>(it, *fixed_p, cnt);
        auto& opcode = MCODE_T::get(reader.get_fixed(sizeof(MCODE_T::index)));
        
        auto args   = serial_args(reader, opcode);
        auto code_p = args.code_p;

        // print "mcode name"
        os << opcode.defn().name();
        
        // ...print machine code...
        os << std::hex << " " << +*code_p;
        if (opcode.opc_long)
            os << "'" << +*++code_p;

        // ...and args
        auto delim = " : ";
        for (auto& arg : args)
        {
            os << delim << arg;
            delim = ",";
        }

#endif
    }

    op_size_t calc_size(Iter it, uint16_t cnt, core::core_fits const& fits) override
    {
        // deserialize insn data
        // format:
        //  1) opcode index
        //  2) opcode binary code (word or long)
        //  3) serialized args
#if 0
        auto  reader = z80_data_reader(it, *fixed_p, cnt);
        auto& opcode = z80_opcode_t::get(reader.get_fixed(M_SIZE_WORD));
        //auto  op_p   = reader.get_fixed_p(opcode.opc_long ? M_SIZE_LONG : M_SIZE_WORD);
        auto  args   = serial_args{reader, opcode};

        // base instruction size
        op_size_t new_size = sizeof(uint16_t) * (1 + opcode.opc_long);
       
        // evaluate args with new `fits`
        for (auto& arg : args)
        {
            auto old_mode = arg.mode();
            new_size += arg.size(fits);
            if (arg.mode() != old_mode)
                args.update_mode(arg);      // XXX fold into TGT method...Not called for Z80...
        }
#else
        //using MCODE_T = z80_opcode_t;

        auto  reader = tgt_data_reader<mcode_size_t>(it, *fixed_p, cnt);
        auto& opcode = MCODE_T::get(reader.get_fixed(sizeof(MCODE_T::index)));
        
        auto args   = serial_args(reader, opcode);

        // base instruction size
        op_size_t new_size = opcode.emit_size();
       
        // evaluate args with new `fits`
        for (auto& arg : args)
        {
            auto old_mode = arg.mode();
            new_size += arg.size(fits);
            //if (arg.mode() != old_mode)
            //    args.update_mode(arg);      // XXX fold into TGT method...Not called for Z80...
        }
#endif
        return new_size;
    }

    void emit(Iter it, uint16_t cnt, core::emit_base& base, core::core_expr_dot const *dot_p) override
    {
        // deserialze insn data
        // format:
        //  1) opcode index
        //  2) opcode binary code (word or long)
        //  3) serialized args
        auto  reader = tgt_data_reader<mcode_size_t>(it, *fixed_p, cnt);
        auto& opcode = MCODE_T::get(reader.get_fixed(sizeof(MCODE_T::index)));

        auto args   = serial_args(reader, opcode);
        
        opcode.emit(base, args.code_p, args, dot_p);
    }
};
}
#endif
