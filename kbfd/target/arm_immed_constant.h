#ifndef KBFD_TARGET_ARM_IMMED_CONSTANT_H
#define KBFD_TARGET_ARM_IMMED_CONSTANT_H

// Per ARM V7-AR, Section A5.2.4: calculate modified immediate constant
// 8-bits, shifted, preferring lowest shift "value"

#include <utility>      // std::pair
#include <iostream>     // debug messages

namespace kbfd::arm
{
struct arm_immed_constant
{
    // find MSB "bit-pair", ie shift must be even
    static constexpr uint16_t find_msb_pair(uint32_t v) 
    {
        uint16_t shift{};
        if (v > 0xffff) { shift += (1 << 4); v >>= (1 << 4); }
        if (v > 0xff)   { shift += (1 << 3); v >>= (1 << 3); }
        if (v > 0xf)    { shift += (1 << 2); v >>= (1 << 2); }
        if (v > 0x3)    { shift += (1 << 1);                 }
        return shift;
    }

    // return encoded value & residual 
    std::pair<uint16_t, uint32_t> encode_alu_immed(uint32_t v) const
    {
        //std::string f {__func__};
        //std::cout << f + ": v = " << std::hex << v << std::endl;

        // short circuit for small positive number: no shift, no residual
        if (v <= 0xff) return {v, 0};

        // find even `shift` which locates MSB in `v`
        // here actually working with "shift * 2". Correct when normalizing.
        // precompensate to allow 8 significant bits thru mask 
        auto shift    = find_msb_pair(v) - 6; 
        auto residual = v;          // save initial `v` to calculate residual
        
        // because definition uses "rotation", any of six LSBs can complicate 
        // calculation of shift/mask. Test & short circuit no-LSBs set

        // the `32` is for 32-bit rotation
        // the `8` compensates for zero-shift value.
        // the `6` compensates for allowing 6-bits of 8 in value
        auto lsb_shift = shift - (32 - 8 - 6);
        if (lsb_shift >= 2)
        {
            uint32_t lsb_mask = ~(~0 << lsb_shift);
            if (auto lsbs = v & lsb_mask)
            {
                residual ^= lsbs;       // clear LSBs from residual

                while (lsb_mask)
                {
                    lsb_mask >>= 2;
                    if (lsbs & ~lsb_mask) break;
                    lsb_shift -= 2;
                }
                
                // update shift with lsb_shift & rotate v
                // written as one line to see if you're paying attention KBH
                v >>= shift += lsb_shift;
                v |= lsbs << (8 - lsb_shift);
            }
            else
                v >>= shift;        // shift not modified by LSBs
        }
        else
        {
            v >>= shift;
        }

        residual &=~ (0xff << shift);   // use shift(x2) to calculate residual
        shift >>= 1;                    // normalize shift for ALU-immed-shift

        // normalize result (per ARM V7-AR) (ie smallest `v`)
        for (auto i = 3; i--; ++shift)
        {
            if (v & 3) break;
            v >>= 2;
        }
        
        //std::cout << f + ": shift = " << std::dec << shift;
        //std::cout << ", value = " << std::hex << v;
        //std::cout << ", result = " << std::hex << ((16-shift) << 8 | v);
        //std::cout << ", residual = " << residual << std::endl;
        return {(16-shift) << 8 | v, residual}; 
    }
};
}

#endif
