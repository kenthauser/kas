#ifndef M68K_M68K_FORMATS_IMPL_H
#define M68K_M68K_FORMATS_IMPL_H


#include "m68k_mcode.h"
#include "target/tgt_format.h"

namespace kas::m68k::opc
{
// tinker-toy functions to put args into various places...
// always paired: insert & extract

// use generic template to generate `mix-in` type
template <unsigned N, typename T>
using fmt_arg = tgt::opc::tgt_fmt_arg<m68k_mcode_t, N, T>;

// use generic bit inserter/extractor
template <unsigned...Ts>
using fmt_generic = tgt::opc::tgt_fmt_generic<m68k_mcode_t, Ts...>;

//
// m68k specific formatters
//

// For `reg_mode` & `fmt_reg`, the validator passes 6-bit mode+reg
// to be inserted/retrieved. `reg_mode` inserts 6 bits. Both
// `reg_mode` & `fmt_reg` retrieve 6 bits. For `fmt_reg`, the 
// 3 msbs (mode) of the retrived value must be ignored

// These formatters are designed to work with the M68K register validators
// reg-mode sets 6-bits.
// NB: MODE_BITS can be set to 1 to allow only general register extract
template <int SHIFT, unsigned WORD = 0, int MODE_OFFSET = 3, unsigned MODE_BITS = 3>
struct fmt_reg_mode
{
    using val_t = m68k_mcode_t::val_t;
    
    // MODE BITS are either 3 or 1 (ie general mode or general register)
    static constexpr auto MODE_BIT_MASK = (1 << MODE_BITS) - 1; 

    static bool insert(uint16_t* op, m68k_arg_t& arg
                     , val_t const *val_p, core::core_expr_dot const *dot_p)
    {
        kas::expression::expr_fits fits;

        // validator returns 6 bits: mode + reg
        auto value    = val_p->get_value(arg);
        auto cpu_reg  = value & 7;
        auto cpu_mode = (value >> 3) & MODE_BIT_MASK;

        value  = cpu_reg  <<  SHIFT;
        value |= cpu_mode << (SHIFT+MODE_OFFSET);

        static constexpr auto MASK = (7 << SHIFT) | (MODE_BIT_MASK << (SHIFT + MODE_OFFSET)); 

        op[WORD]  &= ~MASK;
        op[WORD]  |= value;

        return fits.zero(arg.expr) == fits.yes;
    }
    
    static void extract(uint16_t const* op, m68k_arg_t& arg, val_t const *val_p)
    {
        auto value     = op[WORD];
        auto reg_num   = (value >>  SHIFT)                & 7;
        auto cpu_mode  = (value >> (SHIFT+MODE_OFFSET)) & MODE_BIT_MASK; 
        val_p->set_arg(arg, reg_num | (cpu_mode << 3));
    }

    static void emit(core::emit_base& base, uint16_t *op, m68k_arg_t& arg
                   , val_t const *val_p, core::core_expr_dot const *dot_p)
    {
    }
};

// if MODE_OFFSET is three, just generic with N bits
template <unsigned SHIFT, unsigned WORD, unsigned BITS>
struct fmt_reg_mode<SHIFT, WORD, 3, BITS> : fmt_generic<SHIFT, 3 + BITS, WORD> {};

// insert a "branch displacement"
struct fmt_displacement
{
    using val_t = m68k_mcode_t::val_t;
    
    static bool insert(uint16_t* op, m68k_arg_t& arg
                     , val_t const *val_p, core::core_expr_dot const *dot_p)
    {
        return false;
    }
    
    static void extract(uint16_t const* op, m68k_arg_t& arg, val_t const *val_p)
    {
    }

    // insert relocation in first word
    static void emit(core::emit_base& base, uint16_t *op, m68k_arg_t& arg
                   , val_t const *val_p, core::core_expr_dot const *dot_p)
    {
        // calculate size here
        core::core_fits fits(dot_p);
        m68k_arg_t::op_size_t size;
        val_p->size(arg, 0, fits, size);

        switch (size())
        {
        default:
            throw std::logic_error{"invalid fmt_displacement size"};

        case 2:   // byte offset
        {
            // 8-bits & pc-relative
            static constexpr core::core_reloc reloc { core::K_REL_ADD, 8, true };
            arg.set_mode(MODE_BRANCH_BYTE);
            
            // displacement from pc + 2, size is 1 byte
            base << core::emit_reloc(reloc, -2, 1) << arg.expr;
            break;
        }
        case 4:   // word offset
            arg.set_mode(MODE_BRANCH_WORD);
            break;
        case 6:   // long offset
            arg.set_mode(MODE_BRANCH_LONG);
            *op |= 0xff;            // long offset will follow
            break;
        }
    }
};


// Format PAIRs: Must specify offsets. Default is 3-bits in WORD1
// NB: if single register is specified for `PAIR` it is duplicated in both positions
template <unsigned SHIFT_0, unsigned SHIFT_1, unsigned BITS = 3, unsigned WORD_0 = 1, unsigned WORD_1 = WORD_0>
struct fmt_reg_pair
{
    // assert anticipated cases
    static_assert(BITS == 3 || BITS == 4);
    
    using val_t = m68k_mcode_t::val_t;
    static constexpr auto BITS_MASK = (1 << BITS) - 1;
    static constexpr auto MASK_0 = (BITS_MASK << SHIFT_0);
    static constexpr auto MASK_1 = (BITS_MASK << SHIFT_1);

    static bool insert(uint16_t* op, m68k_arg_t& arg
                     , val_t const *val_p, core::core_expr_dot const *dot_p)
    {
        auto get_reg = [](auto& e)
            {
                auto rp = e.template get_p<m68k_reg_t>();
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
    
    static void extract(uint16_t const* op, m68k_arg_t& arg, val_t const *val_p)
    {
        // XXX if pair is resolved to same reg twice, should disassembler report
        // XXX `REG` or `REG:REG`. I belive all insns assemble the same with
        // XXX REG & REG:REG.
        auto gen_reg = [](unsigned n)
            {
                auto mode = (n & 8) ? MODE_ADDR_REG : MODE_DATA_REG;
                return m68k_reg_t{ mode, n & 7 };
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
    static void emit(core::emit_base& base, uint16_t *op, m68k_arg_t& arg
                   , val_t const *val_p, core::core_expr_dot const *dot_p)
    {
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
    using val_t = m68k_mcode_t::val_t;

    static bool insert(uint16_t* op, m68k_arg_t& arg
                     , val_t const *val_p, core::core_expr_dot const *dot_p)
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
    
    static void extract(uint16_t const* op, m68k_arg_t& arg, val_t const *val_p)
    {
#if 0
        auto value = op[WORD];
        auto reg_num  = (7 << 0) & (value >> SHIFT);
        auto cpu_mode = (7 << 3) & (value >> (SHIFT+MODE_OFFSET-3));
        
        val_p->set_arg(*arg, reg_num | cpu_mode);
#endif
    }

    static void emit(core::emit_base& base, uint16_t *op, m68k_arg_t& arg
                   , val_t const *val_p, core::core_expr_dot const *dot_p)
    {
    }
};

// emac accN stored: (optionally complemented) LSB in word 0, bit 7; MSB in word 1, bit 5
template <bool INVERT_LSB = false>
struct fmt_emac_an
{
    using val_t = m68k_mcode_t::val_t;
    //static constexpr auto MASK = (7 << SHIFT) | (7 << (SHIFT+MODE_OFFSET));

    static bool insert(uint16_t* op, m68k_arg_t& arg
                     , val_t const *val_p, core::core_expr_dot const *dot_p)
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
    
    static void extract(uint16_t const* op, m68k_arg_t& arg, val_t const *val_p)
    {
#if 0
        auto value = op[WORD];
        auto reg_num  = (7 << 0) & (value >> SHIFT);
        auto cpu_mode = (7 << 3) & (value >> (SHIFT+MODE_OFFSET-3));
        
        val_p->set_arg(*arg, reg_num | cpu_mode);
#endif
    }
    static void emit(core::emit_base& base, uint16_t *op, m68k_arg_t& arg
                   , val_t const *val_p, core::core_expr_dot const *dot_p)
    {
    }
};


}
#endif


