#ifndef KAS_TARGET_TGT_OPC_BRANCH_H
#define KAS_TARGET_TGT_OPC_BRANCH_H


#include "target/tgt_opc_base.h"

namespace kas::tgt::opc
{

template <typename MCODE_T>
struct tgt_opc_branch : MCODE_T::opcode_t
{
    using mcode_t = MCODE_T;
    using base_t  = typename mcode_t::opcode_t;
   
    // XXX don't know why base_t types aren't found.
    // XXX expose types & research later
    using insn_t       = typename base_t::insn_t;
    using bitset_t     = typename base_t::bitset_t;
    using arg_t        = typename base_t::arg_t;
    using arg_mode_t   = typename base_t::arg_mode_t;
    using stmt_info_t  = typename base_t::stmt_info_t;
    using stmt_args_t  = typename base_t::stmt_args_t;
    using mcode_size_t = typename base_t::mcode_size_t;
    using op_size_t    = typename base_t::op_size_t;

    using data_t       = typename base_t::data_t;
    using Iter         = typename base_t::Iter;

    OPC_INDEX();
    using NAME = str_cat<typename MCODE_T::BASE_NAME, KAS_STRING("_BRANCH")>;
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
        // get size for this opcode
        if (auto trace = this->trace)
        {
            *trace << "tgt_opc_branch::gen_insn: " << insn.name;
            auto delim = ": ";
            for (auto& arg : args)
            {
                *trace << delim << arg;
                delim = ", ";
            }
            *trace << " ; info = " << stmt_info;
            *trace << std::endl;
        }
        
        // don't bother to trace, know mcode matches
        //mcode.size(args, mcode.sz(stmt_info), data.size, expression::expr_fits{});

        // serialize format (for resolved instructions)
        // 1) mcode index
        // 2) mcode binary data
        // 3) serialized args
        
        auto inserter = base_t::tgt_data_inserter(data);
        inserter(mcode.index);
        auto machine_code = mcode.code(stmt_info);
        auto code_p       = machine_code.data();
        inserter(*code_p);      // one word
        inserter(std::move(args.front().expr));

        //inserter(std::move(args.front()), M_SIZE_AUTO);
        data.size = {0, 6};     // ranges from deleted to long branch/jmp
        return this;

    }
    
    void fmt(data_t const& data, std::ostream& os) const override
    {
        // deserialize insn data
        // format:
        //  1) opcode index
        //  2) destination as expression

        auto  reader = base_t::tgt_data_reader(data);
        auto& mcode  = mcode_t::get(reader.get_fixed(sizeof(MCODE_T::index)));
        auto  code_p = reader.get_fixed_p(mcode.code_size());
        auto& dest   = reader.get_expr();
        
        auto  info   = mcode.extract_info(code_p);
        
        // print "name"
        os << mcode.defn().name();
        
        // ...print opcode...
        os << std::hex << " " << std::setw(mcode.code_size()) << *code_p;

        // ...and args
        os << " : " << dest;

        // ...and info
        os << " ; info = " << info;
    }

    op_size_t calc_size(data_t& data, core::core_fits const& fits) const override
    {
        // deserialize insn data
        // format:
        //  1) opcode index
        //  2) destination as expression

        auto  reader = base_t::tgt_data_reader(data);
        auto& mcode  = mcode_t::get(reader.get_fixed(sizeof(MCODE_T::index)));
        auto  code_p = reader.get_fixed_p(mcode.code_size());
        auto& dest   = reader.get_expr();
        
        auto  info   = mcode.extract_info(code_p);
        info.bind(mcode);

        // get "final" validator (dest always last arg)
        auto& vals    = mcode.defn().vals();
        auto  arg_cnt = vals.size();
        auto  val_p   = typename mcode_t::val_c_t::iter(vals, arg_cnt - 1);

        arg_t arg(arg_mode_t::MODE_DIRECT, dest);
        auto  ok = val_p->size(arg, info, fits, data.size);
        if (ok == fits.yes)
            *code_p |= arg.mode() - arg_mode_t::MODE_BRANCH_BYTE + 1;
        
        return data.size; 
    }

    void emit(data_t const& data, core::emit_base& base, core::core_expr_dot const *dot_p) const override
    {
        auto  reader = base_t::tgt_data_reader(data);
        auto& mcode  = mcode_t::get(reader.get_fixed(sizeof(MCODE_T::index)));
        auto  code_p = reader.get_fixed_p(mcode.code_size());
        auto& dest   = reader.get_expr();
        
        // extract `info` from code_p
        auto  info   = mcode.extract_info(code_p);
        info.bind(mcode);

        // extract `branch mode` from code_p
        using arg_mode_t = typename mcode_t::arg_mode_t;
        auto mode = static_cast<arg_mode_t>((*code_p & 7) + arg_mode_t::MODE_BRANCH_BYTE - 1);

        // generate "args"
        arg_t args[] = {{mode, dest}};

        // emit
        mcode.emit(base, args, info);
    }
};
}

#endif
