#ifndef KAS_TARGET_TGT_VALIDATE_BRANCH_H
#define KAS_TARGET_TGT_VALIDATE_BRANCH_H

//
// NB: Needs to be CRTP to support different offsets based on `TST`
// eg: m68k: '020 supports long branch
// eg: arm:  arm6 support 20 bit branch, not 18  XXX correct specifics

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
 *****************************************************************************/

#include "tgt_validate.h"

namespace kas::tgt::opc
{

template <typename MCODE_T>
struct tgt_val_branch : MCODE_T::val_t
{
    using base_t      = tgt_val_branch;
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
    
    // maximum size of "jump" if branch doesn't fit (see `tgt_opc_branch`)
    constexpr op_size_t initial() const
    {
        return { cfg_min, cfg_max };
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
    // XXX actually a derived() method
    static constexpr next_table branch_info[] =
    {
        { arg_mode_t::MODE_BRANCH + 1,  8, word_size, is_8bit_v }
    ,   { arg_mode_t::MODE_BRANCH + 2, 16, word_size, 2 }
    ,   { arg_mode_t::MODE_BRANCH + 3, 32, word_size, 4 }
    ,   { arg_mode_t::MODE_BRANCH + 4, 64, word_size, 8 }
    };
   
    constexpr next_test get_test(mode_type& mode) const
    {
        std::cout << "get_test: mode = " << std::dec << +mode << std::endl;
        std::cout << "get_test: min = " << +cfg_min << std::endl;
        for (auto* p = branch_info; mode < arg_mode_t::MODE_BRANCH_LAST; ++p)
        {
            std::cout << "get_test: pend_mode = " << +std::get<0>(*p) << std::endl;
            // find entry first not previously tried
            if (std::get<0>(*p) < mode)
                continue;
            if (std::get<3>(*p) < cfg_min)
                continue;
            if (cfg_max < std::get<3>(*p))
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
        //mode = arg_mode_t::MODE_NONE;
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

    mode_type get_mode(arg_t& arg /*, mcode_t const& mc, stmt_info_t const& info */
                    , expr_fits const& fits, op_size_t& op_size) const 
    {
        std::cout << "tgt_val_branch::get_mode: op_size = " << op_size << std::endl;

        // `core_fits` requires "dot". `expr_fits` does not.
        // require "dot" before evaluating branch displacments
        // NB: not completely object-oriented, but it's assembly after all
        if (dynamic_cast<core::core_fits const*>(&fits) == nullptr)
        {
            std::cout << "tgt_val_branch: expr_fits ignored" << std::endl;
            op_size = initial();
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
    fits_result size(arg_t& arg, uint8_t sz
                   , expr_fits const& fits, op_size_t& op_size) const override
    {
        std::cout << "tgt_val_branch::size: op_size = " << op_size << std::endl;
#if 1
#if 1
        // `core_fits` requires "dot". `expr_fits` does not.
        // `expr_fits.disp()` returns maybe. `core_fits.disp()` returns no
        if (fits.disp() == expr_fits::maybe)
        {
            std::cout << "tgt_val_branch: expr_fits ignored" << std::endl;
            op_size += initial();
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
        auto mode = get_mode(arg /*, mc, info */, fits, op_size);
        if (mode < arg_mode_t::MODE_BRANCH)
            return expr_fits::maybe;
#endif

        auto& dest = arg.expr;
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
        std::cout << "tgt_val_branch::size: op_size = " << op_size;
        std::cout << ", mode = " << +mode << std::endl;

        // prepare for inconclusive match
        op_size_t branch_size = initial();     // get min/max
        fits_result result;
        
        // loop thru table to find first match
        do
        {
            // NB: "mode" is updated by reference
            auto [min, max, offset, arg_size] = get_test(mode);

            std::cout << "get_test: min/max/offset/arg_size = "
                      << +min << "/" << +max << "/" << +offset << "/" << +arg_size << std::endl;
#if 0
            // if error -- done
            if (mode == arg_mode_t::MODE_NONE)
            {
                branch_size.min = arg_size;
                result = expression::NO_FIT;
                break;
            }
#endif
            result = fits.disp(dest, min, max, offset);
            // process result of test
            std::cout << " result = " << +result << std::endl;

            if (result == expression::NO_FIT)
            {
                ++mode;
                continue;
            }
            
            // possible fit: set `min` to value
            branch_size.min = arg_size;
            
            // if does fit, also set max to  value
            if (result == expression::DOES_FIT)
                branch_size.max = arg_size;
            break;
        }
        while(mode < arg_mode_t::MODE_BRANCH_LAST);

        std::cout << "tgt_val_branch::size: mode = " << std::dec << +mode;
        std::cout << ", BRANCH + " << mode-arg_mode_t::MODE_BRANCH;
        std::cout << ", LAST = " << +arg_mode_t::MODE_BRANCH_LAST << std::endl;
        // if MODE_BRANCH_LAST, then max size is max()
        if (mode == arg_mode_t::MODE_BRANCH_LAST)
            branch_size.min = branch_size.max;

        op_size += branch_size;
        arg.set_mode(mode);
        return result;
    } 
    
#endif
    //}
    uint8_t cfg_min, cfg_max, shift;
};

template <typename MCODE_T>
struct tgt_val_branch_del : tgt_val_branch<MCODE_T>
{
        using base_t = tgt_val_branch<MCODE_T>;
    using mcode_t     = MCODE_T;

    using arg_t       = typename mcode_t::arg_t;
    using arg_mode_t  = typename mcode_t::arg_mode_t;
    using stmt_info_t = typename mcode_t::stmt_info_t;
    using reg_t       = typename mcode_t::reg_t;
    using reg_class_t = typename reg_t  ::reg_class_t;
    using reg_value_t = typename reg_t  ::reg_value_t;

    using mode_type   = std::underlying_type_t<arg_mode_t>;

    using base_t::base_t;

    fits_result size(arg_t& arg, uint8_t sz
                   , expr_fits const& fits, op_size_t& op_size) const override
    {
        // if MODE_DIRECT or MODE_BRANCH, test for instruction deletion
        if (arg.mode() <= arg_mode_t::MODE_BRANCH)
        {
            // see if `offset` is just this insn
            auto max = op_size.max + base_t::initial().max;
            switch (fits.disp(arg.expr, 0, 0, max))
            {
            case expression::DOES_FIT:
                std::cout << "branch_del: delete" << std::endl;
                op_size = 0;
                return fits.yes;
            case expression::MIGHT_FIT:
                std::cout << "branch_del: maybe" << std::endl;
                op_size.max = max;
                op_size.min = 0;
                return fits.maybe;
            default:
                break;
            }
        }
        return base_t::size(arg, sz, fits, op_size);
    }
};
}
#endif
