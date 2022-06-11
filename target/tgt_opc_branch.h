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
struct tgt_opc_branch : tgt_opc_general<MCODE_T>
{
    using mcode_t  = MCODE_T;
    using base_t   = typename mcode_t::opcode_t;
    using branch_t = tgt_opc_branch;
   
    // expose base_types from templated ARG type
    using insn_t       = typename base_t::insn_t;
    using ok_bitset_t  = typename base_t::ok_bitset_t;
    using arg_t        = typename base_t::arg_t;
    using argv_t       = typename base_t::argv_t;
    using arg_mode_t   = typename base_t::arg_mode_t;
    using stmt_info_t  = typename base_t::stmt_info_t;
    using stmt_args_t  = typename base_t::stmt_args_t;
    using mcode_size_t = typename base_t::mcode_size_t;
    using op_size_t    = typename base_t::op_size_t;
    using val_t        = typename base_t::val_t;

    using data_t       = typename base_t::data_t;
    using Iter         = typename base_t::Iter;

    OPC_INDEX();
    using NAME = string::str_cat<typename MCODE_T::BASE_NAME, KAS_STRING("_BRANCH")>;
    const char *name() const override { return NAME::value; }
    
    // entrypoint from `mcode` method. used by `opc_list`
    fits_result do_size(mcode_t const& mcode
                      , argv_t& args
                      , decltype(data_t::size)& size
                      , expr_fits const& fits
                      , stmt_info_t info) const override
    {
        // ** assume dest is final arg **
        // imply count of args from count of validators
        auto arg_c  = mcode.vals().size();
        auto val_p  = mcode.vals().last();      // dest validator is always last
        auto& arg   = args[arg_c-1];            // reference dest arg

        // initialize base-code size
        size = mcode.base_size();

        // ask displacement validator (always last) to calculate size
        std::cout << "do_branch_size: initial mode: "
                  << std::dec << +arg.mode() << std::endl;
        auto result = val_p->size(arg, info.sz(mcode), fits, size);
        std::cout << "do_branch_size: result mode = "
                  << std::dec << +arg.mode() << std::endl;
        return result;
    }

    // entrypoint from `mcode` method. used by `opc_list`
    // NB: Can be deleted and evaluate all args using base method 
    void do_emit(core::core_emit& base
                       , mcode_t const& mcode
                       , argv_t& args
                       , stmt_info_t info) const override
    {
        // NB: don't have `code_p`. must use base_t method
        base_t::do_emit(base, mcode, args, info);
    }
    
    // mimic argument retrieval from `tgt_opc_general`
    // can "simplify" generation of branch instructions
    void emit(data_t const& data
            , core::core_emit& base
            , core::core_expr_dot const *dot_p) const override
    {
        // test for deleted instruction
        if (!data.size())
            return;
        
        // deserialze insn data
        // format:
        //  1) opcode index
        //  2) opcode binary code (word or long)
        //  3) serialized args
        //
        // NB: regenerate `code_p` because it reflects "initial" modes

        auto  reader = base_t::tgt_data_reader(data);
        auto& mcode  = mcode_t::get(reader.get_fixed(sizeof(mcode_t::index)));
        auto  args   = base_t::serial_args(reader, mcode);

        this->do_emit(base, mcode, args, args.info);
    }
};
}

#endif
