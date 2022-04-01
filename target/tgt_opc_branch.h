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
                            , mcode_size_t          *code_p
                            , expr_t const&          dest
                            , stmt_info_t const&     info
                            , expr_fits const& fits) const 
    {
        // set arg::mode() to `MODE_BRANCH`, and then evaluate
        insert_branch_mode(code_p, arg_mode_t::MODE_BRANCH);
        return do_calc_size(data, mcode, code_p, dest, info, fits);
    }

    virtual void do_calc_size(data_t&                data
                            , mcode_t const&         mcode
                            , mcode_size_t          *code_p
                            , expr_t const&          dest
                            , stmt_info_t const&     info
                            , expr_fits const& fits) const 
    {
        // NB: arg ctors require `kas_token` to set `kas_position_t`
        // for this eval, just set `mode` and `expr`
        arg_t arg;
        arg.expr = dest;
        arg.set_mode(extract_branch_mode(code_p));
        //std::cout << "do_calc_size: mode = " << +arg.mode() << ", arg = " << arg << std::endl;

        // ask displacement validator (always last) to calculate size
        auto dest_val_p = mcode.vals().last();
        dest_val_p->size(arg, mcode, info, fits, data.size);
        //std::cout << "do_calc_size: result mode = " << +arg.mode() << std::endl;

        // save resulting BRANCH_MODE for next iteration (or emit)
        insert_branch_mode(code_p, arg.mode());
    }

    virtual void do_emit     (data_t const&          data
                            , core::core_emit&       base
                            , mcode_t const&         mcode
                            , mcode_size_t          *code_p
                            , expr_t const&          dest
                            , arg_mode_t             arg_mode) const
    {
        // 1. create an "arg" from dest expression
        arg_t arg;
        arg.expr = dest;
        arg.set_mode(arg_mode);
        
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

    static constexpr auto BRANCH_MODE_MASK = 0xf;   // allow 4 bits
    virtual arg_mode_t extract_branch_mode(mcode_size_t *code_p) const
    {
        auto mode  = code_p[0] & BRANCH_MODE_MASK;
        code_p[0] -= mode;
        return static_cast<arg_mode_t>(mode + arg_mode_t::MODE_BRANCH);
    }

    virtual void insert_branch_mode(mcode_size_t *code_p, uint8_t mode) const
    {
        // XXX bits should be clear -- test for now
        if (code_p[0] & BRANCH_MODE_MASK)
        {
            //std::cout << "insert_branch_mode: bits non-zero: " << std::hex
            //          << code_p[0] << std::endl;
            code_p[0] &=~ BRANCH_MODE_MASK;
        }

        // LSBs should already be zero
        code_p[0] += mode - arg_mode_t::MODE_BRANCH;
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
        // NB: don't bother with last
        auto val_iter = mcode.vals().begin();
        auto last_p = &args.back();
        unsigned n = 0;
        for (auto& arg : args)
            if (&arg != last_p)
                mcode.fmt().insert(n++, code_p, arg, &*val_iter++);

        // retrieve destination address (always last)
        auto& dest = args.back().expr;
        
        // calculate initial insn size
        do_initial_size(data, mcode, code_p, dest, stmt_info, expr_fits{});

        // all jump instructions have condition-code/size/etc in first word 
        inserter(*code_p);          // insert machine code: one word

        // if `sizeof(*code_p)` is byte, add second byte 
        if constexpr (sizeof(*code_p) == 1)
            inserter(0, 1);         // second byte
        
        inserter(std::move(dest)); // save destination address
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
        auto  code_p = reader.get_fixed_p(sizeof(mcode_size_t));
        auto& dest   = reader.get_expr();
        auto  mode   = mcode.calc_branch_mode(data.size());

        auto  info   = mcode.extract_info(code_p);
        
        // print "name"
        os << mcode.defn().name();
        
        // ...print opcode...
        os << std::hex << " " << std::setw(mcode.code_size()) << +*code_p;

        // ...destination...
        os << " : " << dest;

        // ...dest arg mode...
        os << ", mode = " << std::dec << +mode;

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
        auto  code_p = reader.get_fixed_p(sizeof(mcode_size_t));
        auto& dest   = reader.get_expr();
        
        auto  info   = mcode.extract_info(code_p);

        do_calc_size(data, mcode, code_p, dest, info, fits);
        return data.size; 
    }

    void emit(data_t const& data, core::core_emit& base, core::core_expr_dot const *dot_p) const override
    {
        auto  reader  = base_t::tgt_data_reader(data);
        auto& m_code  = mcode_t::get(reader.get_fixed(sizeof(MCODE_T::index)));
        auto  code_p  = reader.get_fixed_p(sizeof(mcode_size_t));
        auto& dest    = reader.get_expr();
        
        // extract encoded info from machine code
        auto info     = m_code.extract_info(code_p);
        auto arg_mode = extract_branch_mode(code_p);

        // expand `code` buffer from one word to full value
        auto code_buf = m_code.code(info);  // generate full opcode
        code_buf[0] = *code_p;              // overwrite first word
       
        // check for deleted instruction
        if (data.size())
            do_emit(data, base, m_code, code_buf.data(), dest, arg_mode);
    }
};
}

#endif
