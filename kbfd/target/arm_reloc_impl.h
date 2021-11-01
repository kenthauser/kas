#ifndef KBFD_TARGET_ARM_RELOC_IMPL_H
#define KBFD_TARGET_ARM_RELOC_IMPL_H

#include "arm_reloc_ops.h"

#include <utility>      // std::pair
#include <iostream>

namespace kbfd::arm
{

// Per ARM V7-AR, Section A5.2.4: calculate modified immediate constant
// 8-bits, shifted, preferring lowest shift "value"

// find MSB "bit-pair", ie shift must be even
uint16_t find_msb_pair(uint32_t v)
{
    uint16_t shift{};
    if (v > 0xffff) { shift += (1 << 4); v >>= (1 << 4); }
    if (v > 0xff)   { shift += (1 << 3); v >>= (1 << 3); }
    if (v > 0xf)    { shift += (1 << 2); v >>= (1 << 2); }
    if (v > 0x3)    { shift += (1 << 1);                 }
    return shift;
}

// return encoded value & residual 
std::pair<uint16_t, uint32_t> encode_alu_immed(uint32_t v)
{
    std::string f {__func__};
    std::cout << f + ": v = " << std::hex << v << std::endl;

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
    
    std::cout << f + ": shift = " << std::dec << shift;
    std::cout << ", value = " << std::hex << v;
    std::cout << ", result = " << std::hex << ((16-shift) << 8 | v);
    std::cout << ", residual = " << residual << std::endl;
    return {(16-shift) << 8 | v, residual}; 
}


struct arm_rel_sel12 : k_rel_add_t, reloc_op_subfield<12, 0> 
{
    using base_t = reloc_op_subfield<12, 0>;
    static constexpr value_t u_bit = 1 << 23;

    value_t read(value_t data) const override
    {
#if 0
        encode_alu_immed(0x23);
        encode_alu_immed(0x10b);
        encode_alu_immed(0x113);
        encode_alu_immed(0x143);
        encode_alu_immed(0x103);
        encode_alu_immed(0x204f);
        encode_alu_immed(0x202f);

        encode_alu_immed(0x0000'0234);    // 0xf
        encode_alu_immed(0x0000'2345);    // 0xd
        encode_alu_immed(0x0020'00ff);
        encode_alu_immed(0x0030'00ff);
        encode_alu_immed(0x0028'00ff);
        encode_alu_immed(0x0024'00ff);
        encode_alu_immed(0x0022'00ff);
        encode_alu_immed(0x0021'00ff);
        encode_alu_immed(0x0020'80ff);
        encode_alu_immed(0x0020'40ff);    // value equals 8 bits
        encode_alu_immed(0x0020'20ff);
        encode_alu_immed(0x0020'10ff);
        encode_alu_immed(0x003f'ab00);    // 0x9
        encode_alu_immed(0x003f'abc0);    // 0x9
        encode_alu_immed(0x003f'abcd);    // 0x9
        encode_alu_immed(0xa400'0000);    // 0x4
        
        encode_alu_immed(0xf123'0000);    // 0x1
        encode_alu_immed(0xf123'0001);    // 0x1
        encode_alu_immed(0xf123'0004);    // 0x1
        encode_alu_immed(0xf123'0010);    // 0x1
        encode_alu_immed(0xf123'0040);    // 0x1
        
        encode_alu_immed(0xc123'0000);    // 0x1
        encode_alu_immed(0xc123'0001);    // 0x1
        encode_alu_immed(0xc123'0004);    // 0x1
        encode_alu_immed(0xc123'0015);    // 0x1
        encode_alu_immed(0xc123'0045);    // 0x1

#endif
        auto value = base_t::read(data);
        if (data & u_bit)
            return value;
        return -value;
    }

    value_t write(value_t data, value_t value) const override
    {
        // clear u-bit for negative value, otherwise set it
        if (value < 0)
        {
            data &=~ u_bit;
            value = -value;
        }
        else
            data |= u_bit;
        return base_t::write(data, value);
    }
};

}

#endif