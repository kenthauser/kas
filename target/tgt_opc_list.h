#ifndef KAS_TARGET_TGT_OPC_LIST_H
#define KAS_TARGET_TGT_OPC_LIST_H

#include "tgt_opc_base.h"

namespace kas::tgt::opc
{

template <typename MCODE_T>
struct tgt_opc_list : MCODE_T::opcode_t
{
    using mcode_t = MCODE_T;

    using base_t       = typename mcode_t::opcode_t;
    using stmt_t       = typename mcode_t::stmt_t;
    using insn_t       = typename mcode_t::insn_t;
    using bitset_t     = typename mcode_t::bitset_t;
    using arg_t        = typename mcode_t::arg_t;
    using stmt_info_t  = typename mcode_t::stmt_info_t;
    using stmt_args_t  = typename mcode_t::stmt_args_t;
    using mcode_size_t = typename mcode_t::mcode_size_t;

    using data_t       = typename base_t::data_t;
    using Iter         = typename base_t::Iter;

    // expose internal type in base class (without "this->" prefix)
    using base_t::serial_args;
    
    OPC_INDEX();
    
    using NAME = str_cat<typename MCODE_T::BASE_NAME, KAS_STRING("_LIST")>;
    const char *name() const override { return NAME::value; }
   
    core::opcode *gen_insn(
                 // results of "validate" 
                   insn_t const&  insn
                 , bitset_t&      ok
                 , mcode_t const& mcode
                 , stmt_args_t&&  args
                 , stmt_info_t    stmt_info

                 // and kas_core boilerplate
                 , data_t& data
                 ) override
    {
        // serialize format (for unresolved instructions)
        // 0) fixed area: OK bitset in host order
        // 1) insn index
        // 2) dummy base opcode (store stmt_info & some arg_info)
        // 3) serialized args

        auto inserter = base_t::tgt_data_inserter(data);
        inserter.reserve(0);       // skip fixed area
        
        inserter(insn.index);
        tgt_insert_args(inserter, mcode, std::move(args), stmt_info);
        
        // store OK bitset in fixed area
        data.fixed.fixed = ok.to_ulong();
        return this;
    }

    void fmt(data_t const& data, std::ostream& os) const override
    {
        // deserialize insn data
        // format:
        //  0) fixed area: OK bitset in host order
        //  1) insn index
        //  2) dummy word to hold args
        //  3) serialized args

        bitset_t ok(data.fixed.fixed);

        auto  reader = base_t::tgt_data_reader(data);
        reader.reserve(0);      // skip fixed area

        auto& insn =  insn_t::get(reader.get_fixed(sizeof(insn_t::index)));
        auto& mc   = *insn.list_mcode_p;
        auto  args =  base_t::serial_args(reader, mc);
        auto  info =  args.info;        // stmt_info

        // print OK bits & name...
        os << ok.to_string().substr(ok.size() - insn.mcodes.size()) << " " << insn.name;

        // ...and args
        auto delim = " : ";
        for (auto& arg : args)
        {
            os << delim << arg;
            delim = ",";
        }
        
        // ...finish with `info`
        std::cout << " ; info: " << info;
    }

    op_size_t calc_size(data_t& data, core::core_fits const& fits) const override
    {
        if (this->trace) *this->trace << std::endl;
        
        // deserialize insn data
        // format:
        //  0) fixed area: OK bitset in host order
        //  1) insn index
        //  2) dummy word to hold args
        //  3) serialized args

        bitset_t ok(data.fixed.fixed);

        auto  reader = base_t::tgt_data_reader(data);
        reader.reserve(0);      // skip fixed area

        auto& insn =  insn_t::get(reader.get_fixed(sizeof(insn_t::index)));
        auto& mc   = *insn.list_mcode_p;
        auto  args =  base_t::serial_args(reader, mc);
        auto& info =  args.info;        // stmt_info deserialized with args

        // evaluate with new `fits`
        insn.eval(ok, args, info, data.size, fits, this->trace);

        // save new "OK"
        data.fixed.fixed = ok.to_ulong();
        return data.size;
    }

    void emit(data_t const& data, core::emit_base& base, core::core_expr_dot const *dot_p) const override
    {
        // deserialize insn data
        // format:
        //  0) fixed area: OK bitset in host order
        //  1) insn index
        //  2) dummy word to hold args
        //  3) serialized args

        bitset_t ok(data.fixed.fixed);

        auto  reader = base_t::tgt_data_reader(data);
        reader.reserve(0);      // skip fixed area

        auto& insn =  insn_t::get(reader.get_fixed(sizeof(insn_t::index)));
        auto& mc   = *insn.list_mcode_p;
        auto  args =  base_t::serial_args(reader, mc);
        auto& info =  args.info;        // stmt_info

        // "find first set" in bitset
        auto index = 0;
        for (auto bitmask = ok.to_ulong(); bitmask; ++index)
            if (bitmask & 1)
                break;
            else
                bitmask >>= 1;

        // select first `machine code` that matches
        auto& selected_mc = *insn.mcodes[index];

#if 0      
        std::cout << "tgt_opc_list::emit: code = ";
        auto code = selected_mc.code(info);
        auto n = selected_mc.code_size()/sizeof(mcode_size_t);
        for (auto p = code.begin(); n--; ++p)
            std::cout << std::hex << +*p << ' ';
        
        auto delim = " ; args : ";
        for (auto& arg : args)
        {
            std::cout << delim << arg << " mode = " << std::dec << +arg.mode();
            delim = ",";
        }
        
        // ...finish with `info`
        std::cout << " ; info: " << info;
        std::cout << std::endl;
#endif        
        selected_mc.emit(base, args, info);
    }
};
}

#endif
