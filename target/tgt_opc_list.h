#ifndef KAS_TARGET_TGT_OPC_LIST_H
#define KAS_TARGET_TGT_OPC_LIST_H

#include "tgt_opcode.h"


namespace kas::tgt::opc
{

template <typename MCODE_T>
struct tgt_opc_list : tgt_opcode<MCODE_T>
{
    using base_t  = tgt_opcode<MCODE_T>;
    using mcode_t = MCODE_T;

    using base_t::serial_args;
   
    using insn_t       = typename mcode_t::insn_t;
    using bitset_t     = typename mcode_t::bitset_t;
    using arg_t        = typename mcode_t::arg_t;
    using stmt_args_t  = typename mcode_t::stmt_args_t;
    using mcode_size_t = typename mcode_t::mcode_size_t;

    using data_t       = typename base_t::data_t;
    using Iter         = typename base_t::Iter;

    OPC_INDEX();
    const char *name() const override { return "TGT_LIST"; }

    core::opcode *gen_insn(
                 // results of "validate" 
                   insn_t const&  insn
                 , bitset_t&      ok
                 , mcode_t const *mcode_p
                 , stmt_args_t&&  args

                 // and kas_core boilerplate
                 , data_t& data
                 ) override
    {
        // process insn for size before saving
        //eval_insn_list(insn, ok, args, data.size, expression::expr_fits(), this->trace);
        insn.eval(ok, args, data.size, expression::expr_fits(), this->trace);

        // serialize format (for unresolved instructions)
        // 0) fixed area: OK bitset in host order
        // 1) insn index
        // 2) dummy zero base opcode (word)
        // 3) serialized args

        auto& di    = data.di();
        auto& fixed = data.fixed;
        auto inserter = base_t::tgt_data_inserter(data);
        inserter.reserve(-1);       // skip fixed area
        
        inserter(insn.index);
        auto& mc = *insn.list_mcode_p;
        tgt_insert_args(inserter, mc, std::move(args));
        
        // store OK bitset in fixed area
        fixed.fixed = ok.to_ulong();
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

        auto& fixed = data.fixed;
        bitset_t ok(fixed.fixed);

        auto  reader = base_t::tgt_data_reader(data);
        reader.reserve(-1);

        auto& insn = insn_t::get(reader.get_fixed(sizeof(insn_t::index)));
        //auto  data_p = reader.get_fixed_p(M_SIZE_WORD);
        auto& mc = *insn.list_mcode_p;
        auto  args = base_t::serial_args(reader, mc);

        // print OK bits & name...
        os << ok.to_string().substr(ok.size() - insn.mcodes.size()) << " " << insn.name;

        // ...and args
        auto delim = " : ";
        for (auto& arg : args)
        {
            os << delim << arg;
            delim = ",";
        }
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

        auto& fixed = data.fixed;
        bitset_t ok(fixed.fixed);

        auto  reader = base_t::tgt_data_reader(data);
        reader.reserve(-1);

        auto& insn = insn_t::get(reader.get_fixed(sizeof(insn_t::index)));
        
        auto& mc   = *insn.list_mcode_p;
        auto  args = base_t::serial_args(reader, mc);

        // evaluate with new `fits`
        //eval_insn_list(insn, ok, args, data.size, fits, this->trace);
        insn.eval(ok, args, data.size, fits, this->trace);

        // save new "OK"
        fixed.fixed = ok.to_ulong();
        return data.size;
    }

    void emit(data_t const& data, core::emit_base& base, core::core_expr_dot const *dot_p) const override
    {
        auto& fixed = data.fixed;
        bitset_t ok(fixed.fixed);
        
        auto  reader = base_t::tgt_data_reader(data);
        reader.reserve(-1);

        auto& insn    = insn_t::get(reader.get_fixed(sizeof(insn_t::index)));
        auto& list_mc = *insn.list_mcode_p;
        auto  args    = serial_args(reader, list_mc);

        // get best match
        op_size_t size; 

        // XXX need core_fits 
        //auto mcode_p = eval_insn_list(insn, ok, args, size, expression::expr_fits(), this->trace);
        auto mcode_p = insn.eval(ok, args, size, expression::expr_fits(), this->trace);
        
        // XXX need to handle error case...

        // get opcode "base" value
        auto& mc = *mcode_p;
        auto code = mc.code();

        // Insert args into machine code "base" value
        auto& fmt         = mc.fmt();
        auto& vals        = mc.vals();
        auto val_iter     = vals.begin();
        auto val_iter_end = vals.end();

        // now that have selected machine code match, must be validator for each arg
        unsigned n = 0;
        for (auto& arg : args)
            fmt.insert(n++, code.data(), arg, &*val_iter++);

        mc.emit(base, code.data(), args, dot_p);
    }
};
}

#endif
