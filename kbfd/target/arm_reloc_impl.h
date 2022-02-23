#ifndef KBFD_TARGET_ARM_RELOC_IMPL_H
#define KBFD_TARGET_ARM_RELOC_IMPL_H

// access generic KBFD relocations
#include "kbfd/kbfd_reloc_ops_kbfd.h"
#include "arm_immed_constant.h"

#include <iostream>

namespace kbfd::arm
{
struct arm_rel_no_tfunc : k_rel_add_t
{
};

// signed 24-bit shifted offset for `B`, `BL` etc
struct arm_rel_a32jump : k_rel_add_t, reloc_op_s_subfield<24>
{
    using base_t = reloc_op_s_subfield<24>;

    value_t extract(value_t data) const override
    {
        return base_t::extract(data) << 2;
    }
        
    // insert to target. return msg if value out-of-range
    const char *insert(value_t& data
                     , value_t  value) const override
    {
        return base_t::insert(data, value >> 2);
    }
};

// for LDR_PC_G0, et seq
// NB: clear THUMB FUNC bit
// XXX should be `reloc_op_u_subfield`
struct arm_rel_a32ldr : reloc_op_s_subfield<12> 
{
    using base_t = reloc_op_s_subfield<12>;
    static constexpr value_t u_bit = 1 << 23;
    
    // support methods for finding MSBs
    static constexpr arm_immed_constant e;

    value_t extract(value_t data) const override
    {
        auto value = base_t::extract(data);
        if (data & u_bit)
            return value;
        return -value;
    }

    const char *insert(value_t& data, value_t value) const override
    {
        // clear u-bit for negative value, otherwise set it
        if (value < 0)
        {
            data &=~ u_bit;
            value = -value;
        }
        else
            data |= u_bit;
        return base_t::insert(data, value);
    }
    
    const char *update(flags_t flags
                     , value_t& accum 
                     , value_t const& addend) const override
    { 
        // don't test for overflow
        accum += addend;
        return {};
    }  
};

// LDR/STR H/SH/SB/D
// Immediate value split into two 4-bits chunks
struct arm_rel_a32ldrs : reloc_op_s_subfield<12>
{
    using base_t = reloc_op_s_subfield<12>;
    static constexpr value_t u_bit = 1 << 23;

    value_t extract(value_t data) const override
    {
        auto value = base_t::extract(data);
        auto msbs  = (value >> 4) & 0xf0;
        value = msbs | (value & 0xf);
        if (data & u_bit)
            return value;
        return -value;
    }

    const char *insert(value_t& data, value_t value) const override
    {
        // clear u-bit for negative value, otherwise set it
        if (value < 0)
        {
            data &=~ u_bit;
            value = -value;
        }
        else
            data |= u_bit;

        if (value &~ 0xff)      // test if out-of-range
            return "E Value out of range: rel_a32ldh";
        value |= value << 4;    // copy MSBs to bits 11-8
        return base_t::insert(data, value & 0xf0f);     // don't modify bits 7-4
    }

};

// LDR/STR coprocessor
// Immediate value is value ASR 2 bits, stored in 8 LSBs
struct arm_rel_a32ldc : reloc_op_s_subfield<8>
{
    using base_t = reloc_op_s_subfield<8>;
    static constexpr value_t u_bit = 1 << 23;

    value_t decode(value_t data) const override
    {
        return data << 2;
    }
    
    const char *encode(value_t& data) const override
    {
        data >>= 2;
        return {};      // XXX wrong error processing
    }
    
    value_t extract(value_t data) const override
    {
        auto value = base_t::extract(data);
        if (data & u_bit)
            return value;
        return -value;
    }

    const char *insert(value_t& data, value_t value) const override
    {
        // clear u-bit for negative value, otherwise set it
        if (value < 0)
        {
            data &=~ u_bit;
            value = -value;
        }
        else
            data |= u_bit;
        return base_t::insert(data, value);
    }

};

struct arm_rel_immed12 : kbfd::k_rel_add_t
{
    static constexpr arm_immed_constant e;

    const char *insert (value_t& data, value_t value)  const override
    {
        auto [encoded, residual] = e.encode_alu_immed(value);
        //std::cout << "arm_rel_a32alu::insert: " << encoded << "/" << residual << std::endl;
        data = (data &~ 0xfff) | encoded;
        // TEST
        return "K Invalid constant";
        return residual ? "K Invalid constant" : nullptr;
    }
};


struct arm_rel_a32alu : kbfd::k_rel_add_t
{
    static constexpr arm_immed_constant e;

    const char *insert (value_t& data, value_t value)  const override
    {
        static constexpr auto op_mask = 0xf << 21;
        static constexpr auto add_op  = 0x4 << 21;
        static constexpr auto sub_op  = 0x2 << 21;

        //unsigned op = (data >> 21) & 0xf;
        //std::cout << "arm_rel_a32alu::insert: op = " << op << std::endl;
        if (value < 0)
        {
            value = -value;     // value < zero: toggle add/sub
            data ^= add_op | sub_op;
        }
        else if (value == 0)
        {
            data &= op_mask;    // force `add` for value == 0
            data |= add_op;
            return {};          // short circuit zero
        }

        auto [encoded, residual] = e.encode_alu_immed(value);
        //std::cout << "arm_rel_a32alu::insert: " << encoded << "/" << residual << std::endl;
        data = (data &~ 0xfff) | encoded;
        // TEST
        return "K Invalid constant";
        return residual ? "K Invalid constant" : nullptr;
    }
};

// R_ARM_V4BX is info for linker, not actual relocation
struct arm_rel_v4bx : kbfd::reloc_op_fns
{
    // assembler should emit RELOC
    bool emit_bare() const override { return true; }
};
    
// XXX arm_rel_movw/movt are incorrect: 
struct arm_rel_movw : k_rel_add_t, reloc_op_u_subfield<4, 12> {};
struct arm_rel_movt : k_rel_add_t, reloc_op_u_subfield<4, 12> {};

//
// Thumb-16 static relocations
//

struct arm_rel_abs5 : k_rel_add_t, reloc_op_u_subfield<5, 6>
{
    using base_t = reloc_op_u_subfield<5, 6>;

    value_t extract(value_t data) const override
    {
        return base_t::extract(data) << 2;
    }
    
    const char *insert(value_t& data, value_t value) const override
    {
        if (value &~ 0x7c)
            return "B Value out of range";
        if (value & 3)
            return "B Value not word aligned";
        return base_t::insert(data, value >> 2);
    }
};

struct arm_rel_pc8    : k_rel_add_t, reloc_op_s_subfield<8> 
{
    using base_t = reloc_op_s_subfield<8, 0>;
#if 0
    value_t insert(value_t data) const override
    {
        return base_t::insert(data) << 2;
    }
    
    const char *insert(value_t& data, value_t value) const override
    {
        if (value &~ 0x3fc)
            return "B Value out of range";
        if (value & 3)
            return "B Value not word aligned";
        return base_t::insert(data, value >> 2);
    }
#endif
};

struct arm_rel_jump8 : k_rel_add_t, reloc_op_s_subfield<8> 
{
    using base_t = reloc_op_s_subfield<8, 0>;
#if 0
    value_t insert(value_t data) const override
    {
        return base_t::insert(data) << 1;
    }
    
    const char *insert(value_t& data, value_t value) const override
    {
        if (value &~ 0x1fe)
            return "B Value out of range";
        if (value & 1)
            return "B Value not word aligned";
        return base_t::insert(data, value >> 1);
    }
#endif
};

struct arm_rel_jump11 : k_rel_add_t, reloc_op_s_subfield<11> 
{
    using base_t = reloc_op_s_subfield<11, 0>;
#if 0
    value_t insert(value_t data) const override
    {
        return base_t::insert(data) << 1;
    }
    
    const char *insert(value_t& data, value_t value) const override
    {
        if (value &~ 0xffe)
            return "B Value out of range";
        if (value & 1)
            return "B Value not word aligned";
        return base_t::insert(data, value >> 1);
    }
#endif
};

struct arm_rel_jump6  : kbfd::k_rel_add_t {};   // THUMB 32

struct arm_rel_thb_call : k_rel_add_t
{
    // insert/insert  22-bits sprea/ over two half-words
    static constexpr auto MASK = (1 << 11) - 1;
    //using base_t = reloc_op_subfield<11, 0>;
#if 0
    value_t insert(value_t data) const override
    {
        std::cout << "arm_rel_thb_call::insert: data = " << std::hex << data;
        auto n  = data & MASK;
             n += (data >> (16 - 11)) & (MASK << 11);
        std::cout << ", result = " << (n << 1) << std::endl;
        return n << 1;
    }
    
    const char *insert(value_t& data, value_t value) const override
    {
        std::cout << "arm_rel_thb_call::insert: data = " << std::hex << data;
        std::cout << ", value = " << value;

#if 0
        // XXX negative value not out-of-range
        if (value &~ 0x1ffffff)
            return "B Value out of range";
        if (value & 1)
            return "B Value not word aligned";
#endif
        value >>= 1;
        data += value & MASK;                           // get 11 LSBs
        data += (value << (16 - 11)) & (MASK << 11);    // 11 MSBs
        std::cout << " -> result = " << data << std::endl;
        return {};
    }
#endif
};

}

#endif
