#ifndef KAS_M68K_M68K_EXTENSION_IMPL_H
#define KAS_M68K_M68K_EXTENSION_IMPL_H

#include "m68k_extension_t.h"
#include "m68k_arg.h"

namespace kas::m68k
{
auto m68k_extension_t::size(m68k_arg_t& arg, expression::expr_fits const *fits_p)
    -> op_size_t
{
    // copy result code enum to namespace
    constexpr auto DOES_FIT  = expression::fits_result::DOES_FIT;
    constexpr auto MIGHT_FIT = expression::fits_result::MIGHT_FIT;
    constexpr auto NO_FIT    = expression::fits_result::NO_FIT;

    // calculate size occupied by index displacement
    // XXX NB: must rework for `PC_INDEX`. Also handle zpc base register
    auto size_for_index = [&](auto disp, auto const& expr, auto& size) -> uint8_t
        {
            // if `fits` set, check size of argument
            if (fits_p)
            {
                // test brief first
                if (brief_ok())
                {
                    if (fits_p->fits<int8_t>(expr) == DOES_FIT)
                        return M_SIZE_NONE;     // convert to brief
                }
                
                // test zero next 
                auto zero_fits = fits_p->zero(expr);
                if (zero_fits == DOES_FIT)
                    return M_SIZE_ZERO;     // save in expr

                // if word fits, don't need long
                auto word_fits = fits_p->fits<int16_t>(expr);
                auto disp  = (word_fits == DOES_FIT) ? M_SIZE_WORD : M_SIZE_LONG;
                
                op_size_t disp_size = (disp == M_SIZE_LONG) ? 4 : 2; 
                if (zero_fits != NO_FIT)
                    disp_size.min = 0;
                    
                size += disp_size;
                return disp;
            }

            // XXX temp. size should not be called w/o fits
            switch (disp)
            {
                case M_SIZE_ZERO:
                    break;
                case M_SIZE_WORD:
                    size += 2;
                    break;
                case M_SIZE_LONG:
                    size += 4;
                default:
                    throw std::logic_error{"size_for_index"};
            }
            return disp;
        };

    // size of extension word
    op_size_t result = 2;
   
    // out-of-range branch errors out at `emit`
    if (is_brief())
        return result;          // nothing to see here...

    // NB: `disp_size` is bitfield, can't be passed by reference
    disp_size = size_for_index(disp_size, arg.expr, result);
    
    if (outer())
        mem_size  = size_for_index(mem_size, arg.outer, result);
    
    return result;
}

void m68k_extension_t::emit(core::emit_base& base, m68k_arg_t const& arg, uint8_t sz) const
{
    // calculate common `brief` and `full` formatted values
    auto base_value = [&]() -> uint16_t
        {
            // construct "brief" extension word
            auto   value  = reg_num      << 4;
                   value |= !reg_is_word << 3;
                   value |= reg_scale    << 1;

            return value <<= 8;
        };

    // calculate bytes to emit for each displacement size
    auto disp_to_bytes = [](auto disp)
        {
            switch (disp)
            {
                default:
                case M_SIZE_NONE:
                case M_SIZE_ZERO:
                    return 0;
                case M_SIZE_WORD:
                    return 2;
                case M_SIZE_LONG:
                    return 4;
            }
        };

    if (is_brief())
    {
        // lower byte is 8-bit signed expression, not pc-relative
        static constexpr core::core_reloc reloc { core::K_REL_ADD, 8, false };
        base << core::emit_reloc(reloc, 0, 1) << arg.expr << base_value();
    }

    else
    {
        // get full index word
        uint16_t value  = base_value() | 0x100;      // show not brief
                 value |= base_suppr     << 7;
                 value |= !has_index_reg << 6;
                 value |= disp_size      << 4;
                 value |= is_post_index  << 2;
                 value |= mem_size       << 0;

        // emit base word, followed by displacements, if indicated
        base << value;
        if (auto n = disp_to_bytes(disp_size))
            base << core::set_size(n) << arg.expr;
        if (auto n = disp_to_bytes(mem_size))
            base << core::set_size(n) << arg.outer;
    }
}



}

#endif

