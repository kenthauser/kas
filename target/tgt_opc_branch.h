#ifndef KAS_TARGET_TGT_OPC_BRANCH_H
#define KAS_TARGET_TGT_OPC_BRANCH_H

// Select format for a "branch"
//
// Many processors have different formats for "branch" instructions,
// depending on the branch distance. The machine-code and branch
// offset formats are determined by examining the offset during relax
// in the `size` method. The `size` of the resulting machine code is
// stored in `size`, which is the "state" of the opcode.
//
// ** OVERRIDE METHODS **
//

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

    // methods to override
    static constexpr auto max_insn = sizeof(expression::e_data_t);
    static constexpr auto max_addr = sizeof(expression::e_addr_t);

    virtual void do_initial_size(data_t&            data
                               , mcode_t const&     mcode
                               , expr_t const&      dest
                               , stmt_info_t const& info) const
    {
        // MIN: allow deletion of insn
        static constexpr auto MIN_SIZE = 0;
        // MAX: one word insn + branch as full size address word
        static constexpr auto MAX_SIZE = sizeof(expression::e_data_t) + 
                                         sizeof(expression::e_addr_t);
        data.size = { MIN_SIZE, MAX_SIZE };
    }


    virtual void do_calc_size(data_t&                data
                            , mcode_t const&         mcode
                            , mcode_size_t          *code_p
                            , expr_t const&          dest
                            , stmt_info_t const&     info
                            , core::core_fits const& fits) const 
    {
        // default: set min -> max
        data.size.min = data.size.max;
    }


    virtual void do_emit     (data_t const&          data
                            , core::emit_base&       base
                            , mcode_t const&         mcode
                            , mcode_size_t          *code_p
                            , expr_t const&          dest
                            , stmt_info_t const&     info) const
    {
        // default: emit opcode words + displacement from end of insn
        auto words = mcode.code_size()/sizeof(mcode_size_t);
        while (words--)
            base << *code_p++;
        base << core::emit_disp(max_addr, -max_addr) << dest;
    }


    // generic methods
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
        
        // serialize format (for branch instructions)
        // 1) mcode index
        // 2) mcode binary data
        // 3) destination 
        
        auto inserter = base_t::tgt_data_inserter(data);
        inserter(mcode.index);
        auto machine_code = mcode.code(stmt_info);
        auto code_p       = machine_code.data();

        // Insert args into machine code "base" value
        auto val_iter = mcode.vals().begin();
        unsigned n = 0;
        for (auto& arg : args)
            mcode.fmt().insert(n++, code_p, arg, &*val_iter++);

        // retrieve destination address (always last)
        auto& dest = args.back().expr;
        
        // calculate initial insn size
        do_initial_size(data, mcode, dest, stmt_info);

        inserter(*code_p);                  // insert machine code: one word
        inserter(std::move(dest));          // save destination address
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
        os << std::hex << " " << std::setw(mcode.code_size()) << +*code_p;

        // ...destination...
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

        do_calc_size(data, mcode, code_p, dest, info, fits);
        return data.size; 
    }

    void emit(data_t const& data, core::emit_base& base, core::core_expr_dot const *dot_p) const override
    {
        auto  reader = base_t::tgt_data_reader(data);
        auto& mcode  = mcode_t::get(reader.get_fixed(sizeof(MCODE_T::index)));
        auto  code_p = reader.get_fixed_p(mcode.code_size());
        auto& dest   = reader.get_expr();
        
        auto  info   = mcode.extract_info(code_p);
        
        do_emit(data, base, mcode, code_p, dest, info);
    }
};
}

#endif
