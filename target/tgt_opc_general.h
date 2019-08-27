#ifndef KAS_Z80_OPC_GENERAL_H
#define KAS_Z80_OPC_GENERAL_H


#include "tgt_opc_base.h"

namespace kas::tgt::opc
{
using namespace kas::core::opc;

using op_size_t = kas::core::opcode::op_size_t;

template <typename MCODE_T>
struct tgt_opc_general : tgt_opc_base<MCODE_T>
{
    using base_t  = tgt_opc_base<MCODE_T>;
    using mcode_t = MCODE_T;
   
    // XXX don't know why base_t types aren't found.
    // XXX expose types & research later
    using insn_t       = typename base_t::insn_t;
    using bitset_t     = typename base_t::bitset_t;
    using arg_t        = typename base_t::arg_t;
    using stmt_info_t  = typename base_t::stmt_info_t;
    using stmt_args_t  = typename base_t::stmt_args_t;
    using mcode_size_t = typename base_t::mcode_size_t;
    using op_size_t    = typename base_t::op_size_t;

    using data_t       = typename base_t::data_t;
    using Iter         = typename base_t::Iter;

    OPC_INDEX();

    using NAME = str_cat<typename MCODE_T::BASE_NAME, KAS_STRING("_GEN")>;
    const char *name() const override { return NAME::value; }
   
    core::opcode *gen_insn(
                 // results of "validate" 
                   insn_t const&  insn
                 , bitset_t&      ok
                 , mcode_t const *mcode_p
                 , stmt_args_t&&  args
                 , stmt_info_t    stmt_info

                 // and opcode data
                 , data_t&  data
                 ) override
    {
        // get size for this opcode
        auto& op = *mcode_p;
        
        if (auto trace = this->trace)
        {
            *trace << "tgt_opc_general::gen_insn: " << insn.name;
            auto delim = ": ";
            for (auto& arg : args)
            {
                *trace << delim << arg;
                delim = ", ";
            }
            *trace << std::endl;
        }
        
        // don't bother to trace, know mcode matches
        op.size(args, mcode_p->sz(stmt_info), data.size, expression::expr_fits{});

        // serialize format (for resolved instructions)
        // 1) mcode index
        // 2) mcode binary data
        // 3) serialized args
        
        auto inserter = base_t::tgt_data_inserter(data);
        inserter(op.index);
        tgt_insert_args(inserter, op, std::move(args), stmt_info);
        return this;
    }
    
    void fmt(data_t const& data, std::ostream& os) const override
    {
        // deserialize insn data
        // format:
        //  1) opcode index
        //  2) opcode binary code (word or long)
        //  3) serialized args
        
        auto  reader = base_t::tgt_data_reader(data);
        auto& mcode  = MCODE_T::get(reader.get_fixed(sizeof(MCODE_T::index)));
        
        auto args    = base_t::serial_args(reader, mcode);
        auto code_p  = args.code_p;

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
    }

    op_size_t calc_size(data_t& data, core::core_fits const& fits) const override
    {
        // deserialize insn data
        // format:
        //  1) opcode index
        //  2) opcode binary code (word or long)
        //  3) serialized args

        auto  reader = base_t::tgt_data_reader(data);
        auto& mcode  = MCODE_T::get(reader.get_fixed(sizeof(MCODE_T::index)));
        
        auto args    = base_t::serial_args(reader, mcode);
        auto info    = args.info;
        info.bind(mcode);

        // base instruction size
        data.size = mcode.base_size();
       
        // evaluate args with new `fits`
        for (auto& arg : args)
            data.size += arg.size(info, &fits);

        // if resolved, write back new sizes
        if (data.size.is_relaxed())
            args.update();
        
        return data.size;
    }

    void emit(data_t const& data, core::emit_base& base, core::core_expr_dot const *dot_p) const override
    {
        // deserialze insn data
        // format:
        //  1) opcode index
        //  2) opcode binary code (word or long)
        //  3) serialized args
        auto  reader = base_t::tgt_data_reader(data);
        auto& mcode  = MCODE_T::get(reader.get_fixed(sizeof(MCODE_T::index)));
        auto args    = base_t::serial_args(reader, mcode);

        mcode.emit(base, args, args.info);
    }
};
}
#endif
