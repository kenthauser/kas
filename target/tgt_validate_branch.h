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

template <typename MCODE_T>
struct tgt_val_branch_t : MCODE_T::val_t {};

template <typename Derived, typename MCODE_T>
struct tgt_val_branch : tgt_val_branch_t<MCODE_T>
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

    using mode_type   = std::underlying_type_t<arg_mode_t>;

    static constexpr auto word_size = sizeof(typename mcode_t::mcode_size_t);
    static constexpr auto is_8bit_v = word_size == 1;

    // default min: no-delete (non-zero), max: zero = code_size() + sizeof(addr)
    // NB: config sizes are not used directly, but thru CRTP functions
    constexpr tgt_val_branch(uint8_t cfg_min = word_size
                           , uint8_t cfg_max = sizeof(expression::e_addr_t)
                           , uint8_t shift   = 0     // don't shift offset
                           )
                   : cfg_min{cfg_min}, cfg_max{cfg_max}, shift{shift} {}
    

    /***********************************************************************
     *
     * Validator configuration section
     *
     * use methods to specify sizes, so CRTP can override
     *
     * NB: when referenced, all methods in this section must be prefixed
     * with `derived().` to allow CRTP override
     *
     **********************************************************************/
 
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

    // CRTP cast
    auto constexpr& derived() const
        { return *static_cast<derived_t const*>(this); }
   
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

    using fits_min_t = typename expr_fits::fits_min_t;
    using fits_max_t = typename expr_fits::fits_max_t;

    // values are: mode, bits, offset, arg_size
    using next_table = std::tuple<uint8_t, uint8_t, int8_t, uint8_t>;

    // values are: min, max, offset, arg_size
    using next_test  = std::tuple<fits_min_t    // `fits` args
                                , fits_max_t
                                , int8_t        // offset
                                , uint8_t       // size of arg if OK
                                >;


    // set default: 1byte, 2byte, 4byte, 8byte
    // entries limited by "MODE_LAST"
    // XXX should probably be `meta::list<>`
    static constexpr next_table branch_info[] =
    {
        { arg_mode_t::MODE_BRANCH + 1,  8, word_size, is_8bit_v }
    ,   { arg_mode_t::MODE_BRANCH + 2, 16, word_size, 2 }
    ,   { arg_mode_t::MODE_BRANCH + 3, 32, word_size, 4 }
    ,   { arg_mode_t::MODE_BRANCH + 4, 64, word_size, 8 }
    };
   
    constexpr next_test get_test(mode_type& mode, unsigned max) const
    {
        std::cout << "get_test: mode = " << std::dec << +mode << std::endl;
        for (auto* p = branch_info; mode < arg_mode_t::MODE_BRANCH_LAST; ++p)
        {
            std::cout << "get_test: pend_mode = " << +std::get<0>(*p) << std::endl;
            // find entry first not previously tried
            if (std::get<0>(*p) < mode)
                continue;
            if (max < std::get<3>(*p))
                break;      // branch exceeds size allows
            
            // convert `next_table` to `next_test`
            mode = std::get<0>(*p);             // update current "mode"
            return {
                  -(1 << (std::get<1>(*p)-1))   // min
                , (1 << std::get<1>(*p)-1)-1    // max 
                , std::get<2>(*p)               // offset
                , std::get<3>(*p)               // size
                };
        }
        mode = arg_mode_t::MODE_NONE;
        return { };                             // no matches
    }

    /***********************************************************************
     *
     * Validator method implementation section
     *
     **********************************************************************/
 
    // Define `tgt_validate` default methods
    fits_result ok(arg_t& arg, expr_fits const& fits) const override
    {
        // simple validator: only allow direct mode
        if (arg.mode() == arg_mode_t::MODE_DIRECT)
            return fits.yes;
        return fits.no;
    }

    mode_type get_mode(arg_t& arg, mcode_t const& mc, stmt_info_t const& info
                    , expr_fits const& fits, op_size_t& op_size) const 
    {
        std::cout << "tgt_val_branch::get_mode: op_size = " << op_size << std::endl;

        // `core_fits` requires "dot". `expr_fits` does not.
        // require "dot" before evaluating branch displacments
        // NB: not completely object-oriented, but it's assembly after all
        if (dynamic_cast<core::core_fits const*>(&fits) == nullptr)
        {
            std::cout << "tgt_val_branch: expr_fits ignored" << std::endl;
            op_size = derived().initial(mc);
            return arg_mode_t::MODE_DIRECT;
        }
        
        // Branch MODE is managed by `tgt_opc_branch`
        // get underlying type so math on enum works
        auto  mode = static_cast<mode_type>(arg.mode());
        
        // if not managed by `tgt_opc_branch`, mode will be "DIRECT"
        if (mode < arg_mode_t::MODE_BRANCH)
            mode = arg_mode_t::MODE_BRANCH;

        return mode;
    }

    // calculate size of branch. Use support routine common with `tgt_opc_branch`
    // `do_size` returns insn size. Convert to arg size
    fits_result size(arg_t& arg, mcode_t const&  mc, stmt_info_t const& info
                   , expr_fits const& fits, op_size_t& op_size) const override
    {
        std::cout << "tgt_val_branch::size: op_size = " << op_size << std::endl;
#if 0
        // `core_fits` requires "dot". `expr_fits` does not.
        // require "dot" before evaluating branch displacments
        // NB: not completely object-oriented, but it's assembly after all
        if (dynamic_cast<core::core_fits const*>(&fits) == nullptr)
        {
            std::cout << "tgt_val_branch: expr_fits ignored" << std::endl;
            op_size = derived().initial(mc);
            return expr_fits::maybe;
        }
        
        std::cout << "tgt_val_branch: core_fits: try resolve" << std::endl;

        // Branch MODE is managed by `tgt_opc_branch`
        // get underlying type so math on enum works
        auto  mode = static_cast<mode_type>(arg.mode());
        
        // if not managed by `tgt_opc_branch`, mode will be "DIRECT"
        if (mode < arg_mode_t::MODE_BRANCH)
            mode = arg_mode_t::MODE_BRANCH;
#else
        auto mode = get_mode(arg, mc, info, fits, op_size);
        if (mode < arg_mode_t::MODE_BRANCH)
            return expr_fits::maybe;
#endif

        // get "base code" size (before displacement arg)
        auto base_code_size = mc.code_size();
        auto& dest = arg.expr;

        // test for insn deletion if appropriate
        if (op_size.min == 0)
        {
            // test for deletion
            // see if `offset` is just this insn
            switch (fits.disp(dest, 0, 0, op_size.max))
            {
            case expression::NO_FIT:
                op_size.min = base_code_size;
                break;
            case expression::DOES_FIT:
                op_size.max = 0;
                return fits.yes;
            default:
                return fits.maybe;
            }
        }
        std::cout << "tgt_val_branch::size: op_size = " << op_size << std::endl;

        // loop thru table to find first match
        // NB: "last" always matches...
        while(mode < arg_mode_t::MODE_BRANCH_LAST)
        {
            // NB: "mode" is set by reference
            auto [min, max, offset, arg_size] = get_test(mode, op_size.max);

            // if error -- done
            if (mode == arg_mode_t::MODE_NONE)
                break;

            auto r = fits.disp(dest, min, max, offset);
            // process result of test
            std::cout << " result = " << +r << std::endl;
            
            // nothing smaller fit -- current test is minimum size
            op_size.min = base_code_size + arg_size;
            switch (r)
            {
                case expression::NO_FIT:
                    ++mode;                     // try next mode
                    continue;
                case expression::DOES_FIT:
                    op_size.max = op_size.min;
                    break;
                default:
                    return expr_fits::maybe;    // might-fit: don't further change min/max
            }
            break;
        }

        std::cout << "tgt_val_branch::size: mode = " << std::dec << +mode << std::endl;
        // nothing fits -- raise min to max
        op_size.min = op_size.max;

        // save "mode": either BRANCH_* if OK, or MODE_NONE if error
        arg.set_mode(mode);
        return mode == arg_mode_t::MODE_NONE ? expr_fits::no : expr_fits::yes;
    } 

    uint8_t cfg_min, cfg_max, shift;
};

template <typename Derived, typename MCODE_T>
struct tgt_val_branch_del : tgt_val_branch<Derived, MCODE_T>
{
#if 0
        // test for insn deletion if appropriate
        if (op_size.min == 0)
        {
            // test for deletion
            // see if `offset` is just this insn
            switch (fits.disp(dest, 0, 0, op_size.max))
            {
            case expression::NO_FIT:
                op_size.min = base_code_size;
                break;
            case expression::DOES_FIT:
                op_size.max = 0;
                return fits.yes;
            default:
                return fits.maybe;
            }
        }
#endif
};
}
#endif
