#ifndef KAS_TARGET_TGT_VALIDATE_BRANCH_H
#define KAS_TARGET_TGT_VALIDATE_BRANCH_H

/******************************************************************************
 *
 * Instruction argument validation base implementation
 *
 *****************************************************************************
 *
 * Declare base validator for displacement branches
 *
 * validate branch with multiple branch offsets
 * handle offset increments in power-2 (ie 1 byte, 2 bytes, 4 bytes)
 * handle offset sizeof zero as delete branch
 * offsets calculated from end of insn. 
 * 
 * NB: this validator is coded slightly differently to allow core methods
 * to be invoked from `tgt_opc_branch` from size() routine...
 *
 *****************************************************************************/

#include "tgt_validate.h"

namespace kas::tgt::opc
{

template <typename Derived, typename MCODE_T>
struct tgt_val_branch : MCODE_T::val_t
{
    using base_t      = tgt_val_branch;
    using derived_t   = Derived;
    using mcode_t     = MCODE_T;

    using arg_t       = typename mcode_t::arg_t;
    using arg_mode_t  = typename mcode_t::arg_mode_t;
    using stmt_info_t = typename mcode_t::stmt_info_t;
    using reg_t       = typename mcode_t::reg_t;
    using reg_class_t = typename reg_t  ::reg_class_t;
    using reg_value_t = typename reg_t  ::reg_value_t;

    using mode_int_t  = std::underlying_type_t<arg_mode_t>;

    static auto constexpr code_size = sizeof(typename mcode_t::mcode_size_t);

    // default no-delete (non-zero), one-word insn + sizeof(addr)
    // NB: config sizes are not used directly, but thru CRTP functions
    constexpr tgt_val_branch(uint8_t cfg_min = code_size
                           , uint8_t cfg_max = (code_size + sizeof(expression::e_addr_t))
                           , uint8_t shift  = 0     // don't shift offset
                           )
                   : cfg_min{cfg_min}, cfg_max{cfg_max}, shift{shift} {}
    

    // CRTP cast
    auto constexpr& derived() const
        { return *static_cast<derived_t const*>(this); }
   
    // Override `tgt_validate` entrypoints
    fits_result ok(arg_t& arg, expr_fits const& fits) const override
    {
        // simple validator: only allow direct mode
        if (arg.mode() == arg_mode_t::MODE_DIRECT)
            return fits.yes;
        return fits.no;
    }

    // calculate size of branch. Use support routine common with `tgt_opc_branch`
    // `do_size` returns insn size. Convert to arg size
    fits_result size(arg_t& arg, mcode_t const & mc, stmt_info_t const& info
                   , expr_fits const& fits, op_size_t& op_size) const override
    {
        // initialize size of insn if not relaxing
        if (op_size.is_relaxed())
            op_size = { derived().min(), derived().max() };

        auto& dest = arg.expr;

        // while min size is in range
        while(op_size.min <= derived().max())
        {
            expression::fits_result r;

            // increase minimum size by code-word size
            uint8_t pending_min_size = op_size.min + code_size;
            
            // validate opcode elegible for deletion
            // different test if trying to delete insn
            if (op_size.min == 0)
            {
                // test for deletion
                // see if `offset` is just this insn
                r = fits.disp(dest, 0, 0, op_size.max);

                // if fails deletion test, continue testing with byte offset
                pending_min_size = derived().min();
            }
            else
            {
                // first word is INSN, rest is offset
                // offset from end of INSN
                // disp_sz calculated to allow 1 word opcode, rest disp
                auto branch_sz = derived().disp_sz(op_size.min);
                r = fits.disp_sz(branch_sz,  dest, op_size.min);
            }

            // process result of test
            std::cout << " result = " << +r << std::endl;
            switch (r)
            {
                case expression::NO_FIT:
                    op_size.min = pending_min_size;
                    continue;
                case expression::DOES_FIT:
                    op_size.max = op_size.min;
                    arg.set_branch_mode(op_size.max);  // update insn mode
                    return expr_fits::yes;      // done
                default:
                    return expr_fits::maybe;    // might-fit: don't further change min/max
            }
        }

        // nothing fits -- raise min to max
        op_size.min = op_size.max;
        return expr_fits::no;
    } 

    // use methods to calculate sizes, so CRTP can override
    // NB: since instance is constexpr, should resolve at compile time
    
    // mininum size of non-deleted insn: 1 byte of opcode, 1 byte of disp
    // NB: if INSN size is over 1 byte, displacement can be part of INSN
    constexpr uint8_t min() const { return std::max<uint8_t>(2, cfg_min); }
    
    // maximum size of insn with largest branch
    constexpr uint8_t max() const { return cfg_max; }

    // convert "insn_sz" to branch sz (in bytes)
    constexpr uint8_t disp_sz(uint8_t insn_sz) const
        { return std::max<uint8_t>(1, insn_sz - code_size); }

    uint8_t cfg_min, cfg_max, shift;
};

}
#endif
