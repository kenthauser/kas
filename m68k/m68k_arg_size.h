#ifndef KAS_M68K_ARG_SIZE_H
#define KAS_M68K_ARG_SIZE_H

#include "m68k_arg.h"
#include "m68k_hw_defns.h"
//#include "m68k_size_defn.h"
#include "expr/expr_fits.h"


namespace kas { namespace m68k
{

    inline auto m68k_arg_t::size(uint8_t sz, expression::expr_fits const& fits)
         -> op_size_t
    {
        // copy result code enum to namespace
        constexpr auto DOES_FIT  = expression::fits_result::DOES_FIT;
        constexpr auto MIGHT_FIT = expression::fits_result::MIGHT_FIT;
        constexpr auto NO_FIT    = expression::fits_result::NO_FIT;

        // calculate size occupied by index displacement
        auto size_for_index = [&](bool brief_ok, auto const& size, auto& expr) -> op_size_t
            {
               // int min, max;
                //std::cout << "m68k_arg_t::size size_for_index: " << expr << " size = " << size;
                //std::cout << " ok = " << brief_ok << std::endl;
                auto make_quick = [&]()
                {
                    //std::cout << "size_for_index::make_quick: arg = " << *this << std::endl;
                    if (auto p = expr.get_fixed_p())
                    {
                        expr = ext.brief_value(*p);
                        set_mode(mode() == MODE_INDEX ? MODE_INDEX_BRIEF : MODE_PC_INDEX_BRIEF);
                    }
                };

                switch (size &~ M_SIZE_POST_INDEX)
                {
                    default:
                    case M_SIZE_ZERO:
                        if (brief_ok)
                            make_quick();
                        return 0;
                    case M_SIZE_BYTE:
                    case M_SIZE_WORD:
                        if (brief_ok)
                        {
                            switch (fits.fits<int8_t>(expr))
                            {
                                case NO_FIT:
                                    break;
                                case DOES_FIT:
                                    make_quick();
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
                        auto f = fits.fits<int16_t>(expr);
                        //std::cout << " NO_FIT = " << std::boolalpha << (f == NO_FIT) << std::endl;
                        if (f == NO_FIT)
                            return 4;

                        // max is long unless word fits
                        short max = (f == DOES_FIT) ? 2 : 4;

                        // test byte if brief mode allowed
                        if (brief_ok)
                        {
                            f = fits.fits<int8_t>(expr);
                            if (f == DOES_FIT)
                            {
                                make_quick();
                                return 0;
                            }
                        }

                        // test zero: (expression suppressed, not quick)
                        f = fits.zero(expr);
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

        switch (mode()) {
            case MODE_IMMED:
                return immed_info(sz).sz_bytes;

            // can modify DISP to indirect or index
            case MODE_ADDR_DISP:
                switch (fits.zero(expr)) {
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

                switch (fits.fits<int16_t>(expr)) {
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
                // if full index, can promote, otherwise
                // generate an error at emit.
                if (!hw::cpu_defs[hw::index_full{}])

                    return { 2, 2 };

                switch (fits.fits<int16_t>(expr)) {
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
                switch (fits.disp<int16_t>(expr, 2)) {
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
                switch (fits.fits<int16_t>(expr)) {
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
            case MODE_MOVEP:
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

    m68k_arg_mode inline m68k_arg_t::mode_normalize() const
    {
        // "normalize" some modes for output in opcode
        switch (mode()) {
        default:
            return mode();
        case MODE_INDEX_BRIEF:
            return MODE_INDEX;
        case MODE_MOVEP:
            return MODE_ADDR_DISP;
        case MODE_DIRECT:
        case MODE_DIRECT_ALTER:
            return MODE_DIRECT_SHORT;
        case MODE_PC_INDEX_BRIEF:
            return MODE_PC_INDEX;
        }
    }

    uint8_t inline m68k_arg_t::cpu_mode() const
    {
        // store MODE_DIRECT as PC_DISP
        if (mode() == MODE_DIRECT)
            return 7;

        auto normalized = mode_normalize();
        if (normalized < 7)
            return normalized;
        if (normalized <= MODE_IMMED)
            return 7;
        return -1;
    }

    uint8_t inline m68k_arg_t::cpu_reg() const
    {
        // store MODE_DIRECT as PC_DISP
        if (mode() == MODE_DIRECT)
            return 2;

        auto normalized = mode_normalize();
        if (normalized < 7)
            return reg_num;
        if (normalized <= MODE_IMMED)
            return normalized - 7;
        return 0;
    }


}}


#endif
