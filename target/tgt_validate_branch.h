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
 * The branch machine code is assumed to be fixed data followed by
 * offset value. Offset is calculated from offset location. 
 * 
 * NB: this validator is coded slightly differently to allow core methods
 * to be invoked from `tgt_opc_branch` from size() routine...
 *
 *************************yy****************************************************/

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

    static auto constexpr word_size = sizeof(typename mcode_t::mcode_size_t);

    // default min: no-delete (non-zero), max: zero = code_size() + sizeof(addr)
    // NB: config sizes are not used directly, but thru CRTP functions
    constexpr tgt_val_branch(uint8_t cfg_min = word_size
                           , uint8_t cfg_max = sizeof(expression::e_addr_t)
                           , uint8_t shift   = 0     // don't shift offset
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
    fits_result size(arg_t& arg, mcode_t const&  mc, stmt_info_t const& info
                   , expr_fits const& fits, op_size_t& op_size) const override
    {
        // initialize size of insn if not relaxing
        if (op_size.is_relaxed())
            op_size = derived().initial(mc);

        auto& dest = arg.expr;

        // while min size is in range
        while(op_size.min <= derived().max(mc))
        {
            expression::fits_result r;

            // increase minimum size by insn-word size
            uint8_t pending_min_size = op_size.min + word_size;
            
            // validate opcode elegible for deletion
            // different test if trying to delete insn
            if (op_size.min == 0)
            {
                // test for deletion
                // see if `offset` is just this insn
                r = fits.disp(dest, 0, 0, op_size.max);

                // if fails deletion test, continue testing with byte offset
                pending_min_size = derived().min(mc);
            }
            else
            {
                // first word is INSN, rest is offset
                // offset from end of INSN
                // disp_sz calculated to allow 1 word opcode, rest disp
                r = derived().disp_sz(mc, fits, dest, op_size.min);
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
                    derived().set_branch_mode(arg, mc, op_size.max);
                    return expr_fits::yes;      // done
                default:
                    return expr_fits::maybe;    // might-fit: don't further change min/max
            }
        }

        // nothing fits -- raise min to max
        op_size.min = op_size.max;
        return expr_fits::no;
    } 

/******************************************************************************
 *
 * Validator configuration section
 *
 * use methods to specify sizes, so CRTP can override
 *
 * NB: when referenced, all methods in this section must be prefixed
 * with `derived().` to allow CRTP override
 *
 *****************************************************************************/
 
    // mininum bytes of non-deleted insn: 1 byte of opcode, 1 byte of disp
    constexpr uint8_t min(mcode_t const& mc) const
    {
        // if insn more than 1 byte, can encode byte offset in insn
        // if insn is single byte, need at least displacement byte
        return mc.base_size() + uint8_t(word_size == 1);
    }
    
    // maximum size of insn with largest branch
    // cfg_max describes largest "offset" value (in bytes)
    constexpr uint8_t max(mcode_t const& mc) const
    { 
        return mc.base_size() + cfg_max;
    }

    // maximum size of "jump" if branch doesn't fit (see `tgt_opc_branch`)
    constexpr op_size_t initial(mcode_t const& mc) const
    {
        if (cfg_min)
            return { min(mc), max(mc) };

        return { 0, max(mc) };
    }

    // convert "insn_sz" to branch sz (in bytes)
    template <typename ARG_T>
    constexpr expression::fits_result disp_sz(mcode_t const&   mc
                                            , expr_fits const& fits
                                            , ARG_T const&     dest
                                            , uint8_t          op_sz_min) const
    {
        auto base = mc.base_size();     // fixed opcode data 
        return fits.disp_sz(op_sz_min - base,  dest, base);
    }

    void set_branch_mode(arg_t& arg, mcode_t const& mc, uint8_t op_size) const
    {
       arg.set_branch_mode(mc.calc_branch_mode(op_size));
    }

    uint8_t cfg_min, cfg_max, shift;
};

}
#endif
