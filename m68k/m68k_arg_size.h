#ifndef KAS_M68K_ARG_SIZE_H
#define KAS_M68K_ARG_SIZE_H

#include "m68k_arg.h"
#include "m68k_hw_defns.h"
#include "expr/expr_fits.h"


namespace kas::m68k
{

auto m68k_arg_t::size(uint8_t sz, expression::expr_fits const& fits, bool *is_signed_p)
     -> op_size_t
{
    // copy result code enum to namespace
    constexpr auto DOES_FIT  = expression::fits_result::DOES_FIT;
    constexpr auto MIGHT_FIT = expression::fits_result::MIGHT_FIT;
    constexpr auto NO_FIT    = expression::fits_result::NO_FIT;
    
    short min {};
    op_size_t result;

    //std::cout << "m68k_arg_size: " << *this << ", mode = " << std::dec << mode() << std::endl;

    // default to unsigned
    if (is_signed_p) *is_signed_p = false;

    switch (mode())
    {
        case MODE_IMMEDIATE:
    #if 0
            std::cout << "m68k_arg_t::size: immediate: " << std::showpoint << expr << std::endl;

            // special processing if `floating-point` support
            if constexpr (!std::is_void<expression::e_float_t>())
            {
                // if float as immed, check if floating or fixed format
                if (!immed_info(sz).flt_fmt)
                    // if fixed format immed, check if floating value
                    if (auto p = expr.template get_p<expression::e_float_t>())
                    {
                        std::cout << "m68k_arg_t::size: immediate float" << std::endl;
                        auto n = immed_info(sz).sz_bytes;

                        auto msg = expression::ieee754<expression::e_float_t>().ok_for_fixed(*p, n * 8);
                        if (msg)
                            set_error(msg);
                        else
                            e_diag_t::warning(m68k::error_msg::ERR_flt_fixed);
                    }
            }
            if (is_signed) *is_signed = true;
            return derived().immed_info(sz).sz_bytes;
        #endif
            // immediate can be signed
            if (is_signed_p)
                *is_signed_p = true;
            return immed_info(sz).sz_bytes;

        // can modify DISP to indirect or index
        case MODE_ADDR_DISP_LONG:
            if (is_signed_p)
                *is_signed_p = true;
            
            switch (fits.fits<int16_t>(expr))
            {
                case NO_FIT:
                    return 6;       // index + disp
                case DOES_FIT:
                    set_mode(MODE_ADDR_DISP);
                    break;          // FALLSTHRU resolve
                default:
                    // if `expr` == 0, then just straight indirect
                    if (fits.zero(expr) == NO_FIT)
                        return {2, 6};
                    return {0, 6};  // expr: could be zero or very big
            }
            // FALLSTHRU

        case MODE_ADDR_DISP:
            // displacement is signed
            if (is_signed_p)
                *is_signed_p = true;
            
            switch (fits.zero(expr))
            {
                case DOES_FIT:
                    set_mode(MODE_ADDR_INDIR);
                    return 0;
                case NO_FIT:
                    return 2; 
                    break;
                default:
                    return { 0, 2 };
            }
        
        case MODE_PC_DISP_LONG:
            // PC displacement from 2+insn
            switch (fits.disp<int16_t>(expr, 2))
            {
                case NO_FIT:
                    return 6;       // index + disp
                case DOES_FIT:
                    set_mode(MODE_PC_DISP);
                    break;          // FALLSTHRU resolve
                default:
                    // if `expr` == 0, then just straight indirect
                    if (fits.zero(expr) == NO_FIT)
                        return {2, 6};
                    return {0, 6};  // expr: could be zero or very big
            }
            // FALLSTHRU

        // can modify PC_DISP to index
        case MODE_PC_DISP:
            // displacement is signed
            if (is_signed_p)
                *is_signed_p = true;
            
            return 2;

        case MODE_PC_INDEX:
        case MODE_INDEX:
            //std::cout << "m68k_arg_size: index: brief_ok: " << ext.brief_ok();
            //std::cout << " disp_size: " << ext.disp_size << " disp: " << disp;
            //std::cout << " mem_mode: " << ext.mem_mode << " outer: " << outer;
            //std::cout << std::endl;
            if (is_signed_p)
                *is_signed_p = true;        // displacements are signed
            return ext.size(*this, fits, wb_ext_p);

        case MODE_DIRECT:
            // see if PC-relative mode OK.
            // always the first arg, so offset is always 2
            //std::cout << "m68k_arg_size: MODE_DIRECT" << " disp = " << expr;
            
            switch (fits.disp<int16_t>(expr, 2))
            {
                case DOES_FIT:
                    // update mode
                    set_mode(MODE_DIRECT_PCREL);
                    return 2;
                case NO_FIT:
                    break;
                default:
                    return {2, 4};
            }
            
            // FALLSTHRU
        case MODE_DIRECT_ALTER:
            //std::cout << "m68k_arg_size: MODE_DIRECT_ALTER" << " disp = " << expr;

            switch (fits.fits<int16_t>(expr))
            {
                case DOES_FIT:
                    //std::cout << " -> DIRECT_SHORT" << std::endl;
                    set_mode(MODE_DIRECT_SHORT);
                    return 2;
                case NO_FIT:
                    //std::cout << " -> DIRECT_LONG" << std::endl;
                    set_mode(MODE_DIRECT_LONG);
                    return 4;
                default:
                    //std::cout << " (maybe) " << std::endl;
                    break;
            }
            return {2, 4};

        case MODE_DIRECT_PCREL:
            //std::cout << "m68k_arg_size: MODE_DIRECT_PCREL" << " disp = " << expr;
            return 2;

        // modes encoded in CPU
        case MODE_DATA_REG:
        case MODE_ADDR_REG:
        case MODE_ADDR_INDIR:
        case MODE_POST_INCR:
        case MODE_PRE_DECR:
            return 0;

        // modes with single extension word
        case MODE_DIRECT_SHORT:
        case MODE_REGSET:
            return 2;
            
        case MODE_MOVEP:
            if (is_signed_p)
                *is_signed_p = true;
            return 2;

        // modes with two extension words
        case MODE_DIRECT_LONG:
            return 4;

        // dummy modes w/o extension (data stores in opcode)
        case MODE_REG:              // direct non-general register
        case MODE_PAIR:
        case MODE_BITFIELD:
        case MODE_IMMED_QUICK:
        default:
            return 0;
    }
}

}


#endif
