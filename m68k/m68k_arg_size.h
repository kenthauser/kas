#ifndef KAS_M68K_ARG_SIZE_H
#define KAS_M68K_ARG_SIZE_H

#include "m68k_arg.h"
#include "m68k_hw_defns.h"
#include "expr/expr_fits.h"


namespace kas::m68k
{

auto m68k_arg_t::size(uint8_t sz, expression::expr_fits const *fits_p, bool *is_signed)
     -> op_size_t
{
    // copy result code enum to namespace
    constexpr auto DOES_FIT  = expression::fits_result::DOES_FIT;
    constexpr auto MIGHT_FIT = expression::fits_result::MIGHT_FIT;
    constexpr auto NO_FIT    = expression::fits_result::NO_FIT;

    // calculate size occupied by index displacement
    auto size_for_index = [&](bool brief_ok, auto const& size, auto& expr) -> op_size_t
        {
            // convert to "INDEX_BRIEF" mode.
            auto make_brief = [&]
            {
                //std::cout << "size_for_index::make_brief: arg = " << *this << std::endl;
                if (auto p = expr.get_fixed_p())
                {
                    expr = ext.brief_value(*p);
                    set_mode(mode() == MODE_INDEX ? MODE_INDEX_BRIEF : MODE_PC_INDEX_BRIEF);
                }
            };

            // if used for deserializer or disassembler, `expr` & `outer` are zero.
            // return `size` based on examining extension word.
            // NB: assume `M_SIZE_WORD` for `M_SIZE_NONE`, & `M_SIZE_AUTO`.
            // NB: for deserializer, the appropriate `has_expr` bit will be set.
            // NB: should not happen for disassembler.
            if (!fits_p)
            {
                // check for brief extension
                if (ext.is_brief())
                    return 0;

                // add in inner displacement
                switch (ext.disp_size)
                {
                    case M_SIZE_WORD:
                        return 2;
                        break;
                    case M_SIZE_LONG:
                        return 4;
                    default:
                        return 0;;
                }
            }

            switch (size &~ M_SIZE_POST_INDEX)
            {
                default:
                case M_SIZE_ZERO:
                    if (brief_ok)
                        make_brief();
                    return 0;
                case M_SIZE_BYTE:
                case M_SIZE_WORD:
                    if (brief_ok)
                    {
                        switch (fits_p->fits<int8_t>(expr))
                        {
                            case NO_FIT:
                                break;
                            case DOES_FIT:
                                make_brief();
                                return 0;
                            default:
                                return {0, 2};
                        }
                    }
                    return 2;
                case M_SIZE_LONG:
                    return 4;
                case M_SIZE_NONE:
                case M_SIZE_AUTO:
                {
                    // test if word displacement OK
                    auto f = fits_p->fits<int16_t>(expr);
                    //std::cout << " NO_FIT = " << std::boolalpha << (f == NO_FIT) << std::endl;
                    if (f == NO_FIT)
                        return 4;

                    // max is long unless word fits
                    short max = (f == DOES_FIT) ? 2 : 4;

                    // test byte if brief mode allowed
                    if (brief_ok)
                    {
                        f = fits_p->fits<int8_t>(expr);
                        if (f == DOES_FIT)
                        {
                            make_brief();
                            return 0;
                        }
                    }

                    // test zero: (expression suppressed, not quick)
                    f = fits_p->zero(expr);
                    if (f == DOES_FIT)
                        return 0;
                    
                    // min is zero unless zero doesn't fit
                    short min = (f != NO_FIT) ? 0 : 2;
                    return { min, max };
                }
            }
        };

    short min;
    op_size_t result;

    // default to unsigned
    if (is_signed) *is_signed = false;

    switch (mode())
    {
        case MODE_IMMED:
            // immediate can be signed
            if (is_signed)
                *is_signed = true;
            return immed_info(sz).sz_bytes;

        // can modify DISP to indirect or index
        case MODE_ADDR_DISP:
            // displacement is signed
            if (is_signed)
                *is_signed = true;
            
            // for extractor or disassembler: don't check mode
            if (!fits_p)
                return 2;
            
            switch (fits_p->zero(expr))
            {
                case DOES_FIT:
                    set_mode(MODE_ADDR_INDIR);
                    return 0;
                case NO_FIT:
                    min = 2;
                    break;
                default:
                    min = 0;
                    break;
            }

            // if full index, can promote, otherwise
            // generate an error at emit.
            if (!hw::cpu_defs[hw::index_full{}])
                return { min, 2 };

            switch (fits_p->fits<int16_t>(expr)) {
                case DOES_FIT:
                    return { min, 2 };
                case NO_FIT:
                    set_mode(MODE_INDEX);
                    return { 6, 6 };
                default:
                    return { min, 6 };
            }

        // can modify PC_DISP to index
        case MODE_PC_DISP:
            // displacement is signed
            if (is_signed)
                *is_signed = true;
           
            // for extractor or disassembler: don't check mode
            if (!fits_p)
                return 2;
            
            // if full index, can promote, otherwise
            // generate an error at emit.
            if (!hw::cpu_defs[hw::index_full{}])

                return 2;

            switch (fits_p->fits<int16_t>(expr)) {
                case DOES_FIT:
                    return { 2, 2 };
                case NO_FIT:
                    set_mode(MODE_PC_INDEX);
                    return { 6, 6 };
                default:
                    return { 2, 6 };
            }

        case MODE_PC_INDEX:
        case MODE_INDEX:
            //std::cout << "m68k_arg_size: index: brief_ok: " << ext.brief_ok();
            //std::cout << " disp_size: " << ext.disp_size << " disp: " << disp;
            //std::cout << " mem_mode: " << ext.mem_mode << " outer: " << outer;
            //std::cout << std::endl;
            result = 2;
            result += size_for_index(ext.brief_ok(), ext.disp_size, expr);
            result += size_for_index(false, ext.mem_mode, outer);
            return result;

        case MODE_INDEX_BRIEF:
        case MODE_PC_INDEX_BRIEF:
            return 2;

        case MODE_DIRECT:
            // see if PC-relative mode OK.
            // always the first arg, so offset is always 2
            //std::cout << "m68k_arg_size: MODE_DIRECT" << std::endl;
            if (!fits_p)
                return 2;

            switch (fits_p->disp<int16_t>(expr, 2))
            {

                case NO_FIT:
                    set_mode(MODE_DIRECT_ALTER);
                    break;
                case DOES_FIT:
                    return 2;
                default:
                    return {2, 4};
            }

            // FALLSTHRU
        case MODE_DIRECT_ALTER:
            //std::cout << "m68k_arg_size: MODE_DIRECT_ALTER" << " disp = " << disp;
            if (!fits_p)
                return 2;

            switch (fits_p->fits<int16_t>(expr))
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
            if (is_signed)
                *is_signed = true;
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
