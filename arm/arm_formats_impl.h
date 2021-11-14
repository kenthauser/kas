#ifndef ARM_ARM_FORMATS_IMPL_H
#define ARM_ARM_FORMATS_IMPL_H


#include "arm_mcode.h"
#include "arm_formats_opc.h"
#include "target/tgt_format.h"

namespace kas::arm::opc
{
// tinker-toy functions to put args into various places...
// always paired: insert & extract

// ARM5: Addressing Mode 1: Immediate Shifts & Register Shifts
// NB: `val_shift` does encoding/decoding
// XXX should be `fmt_abs`
struct fmt_shifter: arm_mcode_t::fmt_t::fmt_impl
{
    using mcode_size_t = arm_mcode_t::mcode_size_t;
    using val_t        = arm_mcode_t::val_t;
    using arg_t        = arm_mcode_t::arg_t;
  
    bool insert(mcode_size_t* op, arg_t& arg, val_t const *val_p) const override
    {
        auto value = val_p->get_value(arg);
        op[1] |= value;
        op[0] |= value >> 16;
        return true;
    }
    
    void extract(mcode_size_t const* op, arg_t& arg, val_t const *val_p) const override
    {
        auto value = (op[0] << 16) | op[1];
        val_p->set_arg(arg, value);
    }
};


// ARM5: Addressing Mode 1: immediate 12-bit value with 8 significant bits
// relocations: R_ARM_ALU_SB_Gx{_NC}
struct fmt_fixed : arm_mcode_t::fmt_t::fmt_impl
{
    using mcode_size_t = arm_mcode_t::mcode_size_t;
    using val_t        = arm_mcode_t::val_t;
    using arg_t        = arm_mcode_t::arg_t;

    bool insert(mcode_size_t* op, arg_t& arg, val_t const *val_p) const override
    {
        op[1] |= val_p->get_value(arg);     // 12-bits into LSBs
        return true;
    }
    
    void extract(mcode_size_t const* op, arg_t& arg, val_t const *val_p) const override
    {
        val_p->set_arg(arg, op[1]);
    }
#if 0
    void emit(core::core_emit& base, mcode_size_t *op, arg_t& arg, val_t const *val_p) const override
    {
    }
#endif
};

// ARM5: Addressing Mode 2: various forms of indirect in bottom 12 bits + flags
// requires validator `val_indir`
struct fmt_reg_indir :  arm_mcode_t::fmt_t::fmt_impl
{
    using mcode_size_t = arm_mcode_t::mcode_size_t;
    using val_t        = arm_mcode_t::val_t;
    using arg_t        = arm_mcode_t::arg_t;
  
    bool insert(mcode_size_t* op, arg_t& arg, val_t const *val_p) const override
    {
        auto value = val_p->get_value(arg);
        //std::cout << "\nfmt_reg_indir::insert: arg = " << arg << ", value = " << std::hex << value << std::endl;
        op[1] |= value;
        op[0] |= value >> 16;

        // if non-constant expression, need reloc
        return arg.expr.get_fixed_p();
    }
   
    // associated reloc: R_ARM_ABS12
    // use for unresolved immediate offset or conversion of `DIRECT` to PC-REL
    void emit_reloc(core::core_emit& base
                  , mcode_size_t *op
                  , arg_t& arg
                  , val_t const *val_p) const override
    {
        using ARM_REL_OFF12 = kbfd::ARM_REL_OFF12;
        static const kbfd::kbfd_reloc r_ind { ARM_REL_OFF12(), 32, false }; 
        static const kbfd::kbfd_reloc r_dir { ARM_REL_OFF12(), 32, true  }; 

        //std::cout << "\nfmt_reg_indir::emit_reloc: arg = " << arg << std::endl;
        switch (arg.mode())
        {
            // handle DIRECT xlated to PC-REL
            // NB: offset must match `val_indir::ok` value to ensure no overflow
            case arg_t::arg_mode_t::MODE_DIRECT:
                base << core::emit_reloc(r_dir, -8) << arg.expr;
                break;

            // handle REG_INDIR with unresolved offset
            case arg_t::arg_mode_t::MODE_REG_IEXPR:
                base << core::emit_reloc(r_ind) << arg.expr;
                break;
            default:
            
                // internal error
                break;
        }
    }
    
    void extract(mcode_size_t const* op, arg_t& arg, val_t const *val_p) const override
    {
        auto value = (op[0] << 16) | op[1];
        val_p->set_arg(arg, value);
    }
};

struct fmt_movw : arm_mcode_t::fmt_t::fmt_impl
{
    using mcode_size_t = arm_mcode_t::mcode_size_t;
    using val_t        = arm_mcode_t::val_t;
    using arg_t        = arm_mcode_t::arg_t;

    bool insert(mcode_size_t* op, arg_t& arg, val_t const *val_p) const override
    {
        // let reloc code insert value
        return false;
    }
    
    void extract(mcode_size_t const* op, arg_t& arg, val_t const *val_p) const override
    {
        
    }
#if 0
    void emit(core::core_emit& base, mcode_size_t *op, arg_t& arg, val_t const *val_p)
    {
        static const kbfd::kbfd_reloc reloc { kbfd::ARM_REL_MOVW(), 4 };
        base << core::emit_reloc(reloc) << arg.expr;
    }
#endif
};

// ARM5: 24-bit branch
struct fmt_branch24 : arm_mcode_t::fmt_t::fmt_impl
{
    using mcode_size_t = arm_mcode_t::mcode_size_t;
    using val_t        = arm_mcode_t::val_t;
    using arg_t        = arm_mcode_t::arg_t;

    // branch `machine code` insertions handled by `emit_reloc`
    void emit_reloc(core::core_emit& base
                  , mcode_size_t *op
                  , arg_t& arg
                  , val_t const *val_p) const override
    {
        std::cout << "fmt_branch24: emit: arg = " << arg << std::endl;
        // calculate size here
        switch (arg.mode())
        {
        default:
            std::cout << "emit_relocation: bad arg: " << arg << ", mode = " << +arg.mode() << std::endl;
           break;
           throw std::logic_error{"invalid fmt_displacement mode"};

        case arg_t::arg_mode_t::MODE_BRANCH:
        {
            // byte displacement stored in LSBs. Use reloc mechanism
            std::cout << "fmt_displacement: MODE_JUMP" << std::endl;
#if 1           
            // reloc is 24-bits & pc-relative
            static const kbfd::kbfd_reloc r { kbfd::ARM_REL_OFF24(), 24, true }; 
            
            // reloc is from end of 2-byte machine code, stored with 1 byte offset
            base << core::emit_reloc(r, -8, 1) << arg.expr;
#endif
            break;
        }
        case arg_t::arg_mode_t::MODE_CALL:
            std::cout << "fmt_displacement: MODE_CALL" << std::endl;
            break;
        }
    }
};

#if 0
// For `reg_mode` & `fmt_reg`, the validator passes 6-bit mode+reg
// to be inserted/retrieved. `reg_mode` inserts 6 bits. Both
// `reg_mode` & `fmt_reg` retrieve 6 bits. For `fmt_reg`, the 
// 3 msbs (mode) of the retrived value must be ignored

// These formatters are designed to work with the ARM register validators
// reg-mode sets 6-bits.
// NB: MODE_BITS can be set to 1 to allow only general register extract
template <int SHIFT, unsigned WORD = 0, int MODE_OFFSET = 3, unsigned MODE_BITS = 3>
struct fmt_reg_mode
{
    using val_t = arm_mcode_t::val_t;
    // actual word mask
    static constexpr auto MASK      = (7 << SHIFT) | (7 << (SHIFT+MODE_OFFSET));

    // shifted word mask (operates on 6-bit value)
    static constexpr auto MODE_MASK = ((1 << MODE_BITS) - 1) << 3;

    static bool insert(uint16_t* op, arg_t& arg, val_t const *val_p)
    {
        kas::expression::expr_fits fits;
        
        // validator return 6 bits: mode + reg
        auto value    = val_p->get_value(arg);
        auto cpu_reg  = value & 7;
        auto cpu_mode = arg.mode_normalize() & MODE_MASK;

        value  = cpu_reg  <<  SHIFT;
        value |= cpu_mode << (SHIFT+MODE_OFFSET-3);
        
        op[WORD]  &= ~MASK;
        op[WORD]  |= value;

        return fits.zero(arg.expr) == fits.yes;
    }
    
    static void extract(uint16_t const* op, arg_t& arg, val_t const *val_p)
    {
        auto value     = op[WORD];
        auto reg_num   = (value >>  SHIFT)                & 7;
        auto cpu_mode  = (value >> (SHIFT+MODE_OFFSET-3)) & MODE_MASK; 
        val_p->set_arg(arg, reg_num | cpu_mode);
    }
};

// if MODE_OFFSET is three, just generic with N bits
template <unsigned SHIFT, unsigned WORD, unsigned BITS>
struct fmt_reg_mode<SHIFT, WORD, 3, BITS> : fmt_generic<SHIFT, 3 + BITS, WORD> {};

// Format PAIRs: Must specify offsets. Default is 3-bits in WORD1
// NB: if single register is specified for `PAIR` it is duplicated in both positions
template <unsigned SHIFT_0, unsigned SHIFT_1, unsigned BITS = 3, unsigned WORD_0 = 1, unsigned WORD_1 = WORD_0>
struct fmt_reg_pair
{
    // assert anticipated cases
    static_assert(BITS == 3 || BITS == 4);
    
    using val_t = arm_mcode_t::val_t;
    static constexpr auto BITS_MASK = (1 << BITS) - 1;
    static constexpr auto MASK_0 = (BITS_MASK << SHIFT_0);
    static constexpr auto MASK_1 = (BITS_MASK << SHIFT_1);
 
    static bool insert(uint16_t* op, arg_t& arg, val_t const *val_p)
    {
        auto get_reg = [](auto& e)
            {
                auto rp = e.template get_p<arm_reg_t>();
                auto n = rp->value();
                if constexpr (BITS > 3)
                    if (rp->kind() == RC_ADDR)
                        n += 8;
                return n;
            };

        // extract "pair" into two ints
        unsigned reg1, reg2;

        if (arg.mode() == MODE_PAIR)
        {
            reg1 = get_reg(arg.expr);
            reg2 = get_reg(arg.outer);
        }
        else
        {
            // assume General Register
            reg1 = arg.reg_num & 7;
            if constexpr (BITS > 3)
                if (arg.mode() == MODE_ADDR_REG)
                    reg1 += 8;
            reg2 = reg1;
        }
       
        if constexpr (WORD_0 != WORD_1)
        {
            auto code  = op[WORD_0] &= ~MASK_0;
            op[WORD_0] = code | (reg1  << SHIFT_0);
                 code  = op[WORD_1] &= ~MASK_1;
            op[WORD_1] = code | (reg2 << SHIFT_1);
        }
        else
        {
            auto code  = op[WORD_0] &= ~(MASK_0 | MASK_1);
                 code |= reg1 << SHIFT_0;
                 code |= reg2 << SHIFT_1;
            op[WORD_0] = code;
        }
        
        return true;
    }
    
    static void extract(uint16_t const* op, arg_t& arg, val_t const *val_p)
    {
        // XXX if pair is resolved to same reg twice, should disassembler report
        // XXX `REG` or `REG:REG`. I belive all insns assemble the same with
        // XXX REG & REG:REG.
        auto gen_reg = [](unsigned n)
            {
                auto mode = (n & 8) ? MODE_ADDR_REG : MODE_DATA_REG;
                return arm_reg_t{ mode, n & 7 };
            };
            
        auto reg1 = (op[WORD_0] & MASK_0) >> SHIFT_0;
        auto reg2 = (op[WORD_1] & MASK_1) >> SHIFT_1;
        
        // `deserializer` overrides mode. may be overwritten to DATA_REG or ADDR_REG
        // set `reg_num` for these cases. No-op for MODE_PAIR
        arg.reg_num = reg1 & 7;
        
        // generate `MODE_PAIR` arg
        arg.expr  = gen_reg(reg1);
        arg.outer = gen_reg(reg2);
        arg.set_mode(MODE_PAIR);
    }
};

// coldfire MAC throws bits all over the place
// Three register bits are shifted by shift and inserted.
// The fourth general register bit (data/addr) is shifted by SHIFT+OFFSET & inserted.
// The "subword" bit is shifted by by SUB_BIT & inserted in SUB_WORD

// data format from validator: 4 LSBs: general register #, SUB_BIT << 4
template <unsigned SHIFT, int B4_OFFSET, unsigned SUB_BIT, unsigned WORD = 0, unsigned SUB_WORD = 1>
struct fmt_subreg
{
    using val_t = arm_mcode_t::val_t;
    using arg_t = arm_mcode_t::arg_t;

    static bool insert(uint16_t* op, arg_t& arg, val_t const *val_p)
    {
#if 0
        // validator return 6 bits: mode + reg
        auto value = val_p->get_value(arg);
        auto cpu_reg  = value & (7 << 0);
        auto cpu_mode = value & (7 << 3);

        value  = cpu_reg  << SHIFT;
        value |= cpu_mode << (SHIFT+MODE_OFFSET-3);
        
        op[WORD]  &= ~MASK;
        op[WORD]  |= value;
#endif
        return true;
    }
    
    static void extract(uint16_t const* op, arg_t& arg, val_t const *val_p)
    {
#if 0
        auto value = op[WORD];
        auto reg_num  = (7 << 0) & (value >> SHIFT);
        auto cpu_mode = (7 << 3) & (value >> (SHIFT+MODE_OFFSET-3));
        
        val_p->set_arg(*arg, reg_num | cpu_mode);
#endif
    }

};

// emac accN stored: (optionally complemented) LSB in word 0, bit 7; MSB in word 1, bit 5
template <bool INVERT_LSB = false>
struct fmt_emac_an
{
    using val_t = arm_mcode_t::val_t;
    using arg_t = arm_mcode_t::arg_t;
    //static constexpr auto MASK = (7 << SHIFT) | (7 << (SHIFT+MODE_OFFSET));

    static bool insert(uint16_t* op, arg_t& arg, val_t const *val_p)
    {
#if 0
        // validator return 6 bits: mode + reg
        auto value = val_p->get_value(arg);
        auto cpu_reg  = value & (7 << 0);
        auto cpu_mode = value & (7 << 3);

        value  = cpu_reg  << SHIFT;
        value |= cpu_mode << (SHIFT+MODE_OFFSET-3);
        
        op[WORD]  &= ~MASK;
        op[WORD]  |= value;
#endif
        return true;
    }
    
    static void extract(uint16_t const* op, arg_t& arg, val_t const *val_p)
    {
#if 0
        auto value = op[WORD];
        auto reg_num  = (7 << 0) & (value >> SHIFT);
        auto cpu_mode = (7 << 3) & (value >> (SHIFT+MODE_OFFSET-3));
        
        val_p->set_arg(*arg, reg_num | cpu_mode);
#endif
    }
};

#endif


}
#endif


