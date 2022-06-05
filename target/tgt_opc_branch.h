#ifndef KAS_TARGET_TGT_OPC_BRANCH_H
#define KAS_TARGET_TGT_OPC_BRANCH_H

// Format for a "branch"
//
// Many processors have different formats for "branch" instructions,
// depending on the branch distance.
//
// The `tgt_opc_branch` opcode supports these methods using the following
// `mcode` attributes
//
// 0. The first word of the opcode is calculated from supplied arguments,
//    save for the destination. This includes CCODE and DJcc registers.
//
// 1. The `destination` argument must be the last argument.
//
// 2. The `validator` for the destination argument must provide the 
//    size for the "min/max" of the argument 
// 
// 3. The `mcode::calc_branch_mode(uint8_t)` method must convert the
//    object size to branch MODE. This is because `arg.emit()` doesn't
//    have access to object size. This method passes that value via `MODE`
//
// 4. The `formatter` for the `destination` object must understand the
//    `MODE` and how the other values were encoded via the "first word"
//    saved by `tgt_opc_branch`. This information is available via *code_p
//
// Thus, the `opc_branch` opcodes are configured via the DESTINTATION
// validators and formatters.


#include "target/tgt_opc_base.h"
#include "target/tgt_validate_branch.h"

namespace kas::tgt::opc
{

template <typename MCODE_T>
struct tgt_opc_branch : MCODE_T::opcode_t
{
    using mcode_t = MCODE_T;
    using base_t  = typename mcode_t::opcode_t;
   
    // expose base_types from templated ARG type
    using insn_t       = typename base_t::insn_t;
    using bitset_t     = typename base_t::bitset_t;
    using arg_t        = typename base_t::arg_t;
    using argv_t       = typename base_t::argv_t;
    using arg_mode_t   = typename base_t::arg_mode_t;
    using stmt_info_t  = typename base_t::stmt_info_t;
    using stmt_args_t  = typename base_t::stmt_args_t;
    using mcode_size_t = typename base_t::mcode_size_t;
    using op_size_t    = typename base_t::op_size_t;

    using data_t       = typename base_t::data_t;
    using Iter         = typename base_t::Iter;

    OPC_INDEX();
    using NAME = string::str_cat<typename MCODE_T::BASE_NAME, KAS_STRING("_BRANCH")>;
    const char *name() const override { return NAME::value; }

    // methods to override
    static constexpr auto max_insn = sizeof(expression::e_data_t);
    static constexpr auto max_addr = sizeof(expression::e_addr_t);

    virtual void do_initial_size(data_t&             data
                            , mcode_t const&         mcode
                            , expr_t const&          dest
                            , stmt_info_t&           info
                            , expr_fits const& fits) const 
    {
        // calulate base instruction size
        data.size = mcode.code_size();
      
        // set arg::mode() to `MODE_BRANCH`, and then evaluate
        return do_branch_size(data, mcode, dest, info, arg_mode_t::MODE_DIRECT, fits);
    }

    void do_branch_size(data_t&                data
                            , mcode_t const&         mcode
                            , expr_t const&          dest
                            , stmt_info_t&           info
                            , unsigned               arg_mode
                            , expr_fits const& fits) const 
    {
        // NB: arg ctors require `kas_token` to set `kas_position_t`
        // for this eval, just set `mode` and `expr`
        arg_t arg;
        arg.expr = dest;
        arg.set_mode(arg_mode);
        std::cout << "do_branch_size: mode = " << +arg.mode() << ", arg = " << arg << std::endl;

        // initialize base-code size
        data.size = mcode.base_size();

        // ask displacement validator (always last) to calculate size
        auto dest_val_p = mcode.vals().last();
        dest_val_p->size(arg, info.sz(mcode), fits, data.size);
        std::cout << "do_branch_size: result mode = " << +arg.mode() << std::endl;

        // save resulting BRANCH_MODE for next iteration (or emit)
        save_branch(info, arg.mode());
    }

    void do_branch_emit     (data_t const&          data
                            , core::core_emit&       base
                            , mcode_t const&         mcode
                            , mcode_size_t          *code_p
                            , expr_t const&          dest
                            , unsigned               arg_mode) const
    {
        // 1. create an "arg" from dest expression
        arg_t arg;
        arg.expr = dest;
        arg.set_mode(arg_mode);
        
        std::cout << "do_branch_emit: mode = " << std::dec << +arg_mode;
        std::cout << ", arg = " << arg << std::endl;

        // 2. insert `dest` into opcode
        // get mcode validators: displacement always "last" arg
        auto  val_it = mcode.vals().last();
        auto  cnt    = mcode.vals().size();
        auto& fmt    = mcode.fmt();
       
        // insert arg into base insn (via reloc) as required
        // NB: can completely override `*code_p` if required
        if (!fmt.insert(cnt-1, code_p, arg, &*val_it))
            fmt.emit_reloc(cnt-1, base, code_p, arg, &*val_it);

        // 3. emit base code
        auto words = mcode.code_size()/sizeof(mcode_size_t);
        while (words--)
            base << *code_p++;

        // 4. emit `dest`
        arg.emit(base, {});     // sz derived from `MODE`
    }

    // XXX can turn into virtual fn if can get types for reader/writer
    template <typename INSERTER>
    void save_info_mode(INSERTER& inserter, stmt_info_t& info
                      , unsigned mode = {}) const
    {
        // mode saved in `stmt_info` & inits to zero
        // get raw memory buffer to store data
        // NB: memory buffer guaranteed to have proper size and alignment
        inserter.reserve(sizeof(stmt_info_t), alignof(stmt_info_t));
        inserter(info.value());
    }

    template <typename READER>
    std::pair<stmt_info_t *, unsigned> get_info_mode(READER& reader) const
    {
        // get pointer to raw buffer holding `stmt_info`
        // NB: memory buffer guaranteed to have proper size and alignment
        reader.reserve(sizeof(stmt_info_t), alignof(stmt_info_t));
        void *p = reader.get_fixed_p(sizeof(stmt_info_t));
        auto stmt_info_p = static_cast<stmt_info_t *>(p);
        unsigned mode = stmt_info_p->get_raw_branch() + arg_mode_t::MODE_BRANCH;
        return { stmt_info_p, mode };
    }
    
    void save_branch(stmt_info_t& info, unsigned data) const
    {
        // save MODE_BRANCH *offset*, not actual mode
        if (data >= arg_mode_t::MODE_BRANCH)
            info.set_raw_branch(data - arg_mode_t::MODE_BRANCH);
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
        // 2) (stmt_info.ccode << 3) | (branch_mode offset)
        // 3) destination 
        
        auto inserter = base_t::tgt_data_inserter(data);
        inserter(mcode.index);
        
        // single arg is expression
        auto& dest = args[0].expr;
        
        // calculate initial insn size
        do_initial_size(data, mcode, dest, stmt_info, expr_fits{});
        save_info_mode(inserter, stmt_info);
        inserter(std::move(dest)); // save destination address
        return this;
    }
    

    // entrypoint from `mcode` method. Trampoline to `do_branch_size`
    fits_result do_size(mcode_t const& mcode
                      , argv_t& args
                      , decltype(data_t::size)& size
                      , expr_fits const& fits
                      , stmt_info_t info) const override
    {
        return mcode.size(args, info, size, fits, this->trace);
    }

    // entrypoint from `mcode` method. Trampoline to `do_branch_emit`
    void do_emit(core::core_emit& base
                       , mcode_t const& mcode
                       , argv_t& args
                       , stmt_info_t info) const override
    {
        base_t::do_emit(base, mcode, args, info);
    }

    
    void fmt(data_t const& data, std::ostream& os) const override
    {
        // deserialize insn data
        // format:
        //  1) opcode index
        //  2) destination as expression

        auto  reader = base_t::tgt_data_reader(data);
        auto& mcode  = mcode_t::get(reader.get_fixed(sizeof(MCODE_T::index)));
        auto [info_p, mode] = get_info_mode(reader);
        auto& dest   = reader.get_expr();
        
        // print "name"
        os << mcode.defn().name();
        
        // ...print opcode...
        //os << std::hex << " " << std::setw(mcode.code_size()) << +*code_p;

        // ...destination...
        os << " : " << dest;

        // ...dest arg mode...
        os << ", mode = " << std::dec << +mode;

        // ...and info
        os << " ; info = " << *info_p;
    }

    op_size_t calc_size(data_t& data, core::core_fits const& fits) const override
    {
        // deserialize insn data
        // format:
        //  1) opcode index
        //  2) data word (default: `stmt_info`)
        //  3) destination as expression
        
        auto  reader = base_t::tgt_data_reader(data);
        auto& mcode  = mcode_t::get(reader.get_fixed(sizeof(MCODE_T::index)));
        auto [info_p, mode] = get_info_mode(reader);
        auto& dest   = reader.get_expr();
        
        argv_t args;
        args[0] = {arg_mode_t::MODE_DIRECT, dest};

        do_branch_size(data, mcode, dest, *info_p, mode, fits);
        return data.size; 
    }

    void emit(data_t const& data, core::core_emit& base, core::core_expr_dot const *dot_p) const override
    {
        auto  reader = base_t::tgt_data_reader(data);
        auto& mcode  = mcode_t::get(reader.get_fixed(sizeof(MCODE_T::index)));
        auto [info_p, mode] = get_info_mode(reader);
        auto& dest   = reader.get_expr();
        
        // generate code using recovered `info`
        auto code_buf = mcode.code(*info_p);  // generate full opcode
       
        // check for deleted instruction
        if (data.size())
            do_branch_emit(data, base, mcode, code_buf.data(), dest, mode);
    }
};
}

#endif
