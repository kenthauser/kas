#ifndef KAS_Z80_OPC_GENERAL_H
#define KAS_Z80_OPC_GENERAL_H


//#include "z80/z80_stmt_opcode.h"
//#include "z80_opcode_emit.h"
//#include "target/tgt_insn_serialize.h"

#include "tgt_opcode.h"

namespace kas::tgt::opc
{
using namespace kas::core::opc;
//using namespace kas::tgt::opc;

//using args_t = decltype(stmt_z80::args);
using op_size_t = kas::core::opcode::op_size_t;

template <typename MCODE_T>
struct tgt_opc_general : tgt_opcode<MCODE_T>
{
    using base_t  = tgt_opcode<MCODE_T>;
    using mcode_t = MCODE_T;
   
    // XXX don't know why base_t types aren't found.
    // XXX expose types & research later
    using insn_t       = typename base_t::insn_t;
    using bitset_t     = typename base_t::bitset_t;
    using arg_t        = typename base_t::arg_t;
    using stmt_args_t  = typename base_t::stmt_args_t;
    using mcode_size_t = typename base_t::mcode_size_t;
    using op_size_t    = typename base_t::op_size_t;

    // XXX also need to expose `base_t` inherited types
    using Inserter     = typename base_t::Inserter;
    using fixed_t      = typename base_t::fixed_t;
    using Iter         = typename base_t::Iter;

    OPC_INDEX();

    const char *name() const override { return "TGT_GEN"; }
   
    core::opcode& gen_insn(
                 // results of "validate" 
                   insn_t const&  insn
                 , bitset_t&      ok
                 , mcode_t const *mcode_p
                 , stmt_args_t&&  args

                 // and kas_core boilerplate
                 , Inserter&  di
                 , fixed_t&   fixed
                 , op_size_t& insn_size
                 ) override
    {
        // get size for this opcode
        auto& op = *mcode_p;
        
        std::cout << "opc_general::gen_insn: " << insn.name;
        auto delim = ": ";
        for (auto& arg : args)
        {
            std::cout << delim << arg;
            delim = ", ";
        }
        std::cout << std::endl;
        
        op.size(args, insn_size, expression::expr_fits{}, this->trace);

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

        auto  reader = tgt_data_reader<mcode_size_t>(it, *this->fixed_p, cnt);
        auto& mcode  = MCODE_T::get(reader.get_fixed(sizeof(MCODE_T::index)));
        
        auto args   = serial_args(reader, mcode);
        auto code_p = args.code_p;

        // print "mcode name"
        os << mcode.name();
        
        // ...print machine code
        // ...first word
        os << std::hex;

        auto delim = " ";
        auto n = mcode.code_size();
        for(auto n = mcode.code_size(); n > 0; ++code_p)
        {
            os << delim << +*code_p;
            delim = "'";
            n -= sizeof(*code_p);
        }
        
        // ...and args
        delim = " : ";
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

        auto  reader = tgt_data_reader<mcode_size_t>(it, *this->fixed_p, cnt);
        auto& mcode = MCODE_T::get(reader.get_fixed(sizeof(MCODE_T::index)));
        
        auto args   = serial_args(reader, mcode);

        // base instruction size
        op_size_t new_size = mcode.base_size();
       
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
        auto  reader = tgt_data_reader<mcode_size_t>(it, *this->fixed_p, cnt);
        auto& opcode = MCODE_T::get(reader.get_fixed(sizeof(MCODE_T::index)));

        auto args   = serial_args(reader, opcode);
        
        opcode.emit(base, args.code_p, args, dot_p);
    }
};
}
#endif
