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
struct fmt_reg_mode : m68k_mcode_t::fmt_t::fmt_impl
{
    using val_t = m68k_mcode_t::val_t;
    
    // MODE BITS are either 3 or 1 (ie general mode or general register)
    static constexpr auto MODE_BIT_MASK = (1 << MODE_BITS) - 1; 

    bool insert(uint16_t* op, m68k_arg_t& arg, val_t const *val_p) const override
    {
        // validator returns 6 bits: mode + reg
        auto value    = val_p->get_value(arg);
        auto cpu_reg  = value & 7;
        auto cpu_mode = (value >> 3) & MODE_BIT_MASK;

        value  = cpu_reg  <<  SHIFT;
        value |= cpu_mode << (SHIFT+MODE_OFFSET);

        static constexpr auto MASK = (7 << SHIFT) | (MODE_BIT_MASK << (SHIFT + MODE_OFFSET)); 

        op[WORD]  &= ~MASK;
        op[WORD]  |= value;

        // test if expr is zero. return true iff expr == zero
        auto p = arg.expr.get_fixed_p();
        return p && !*p;
    }
    
    void extract(uint16_t const* op, m68k_arg_t& arg, val_t const *val_p) const override
    {
        auto value     = op[WORD];
        auto reg_num   = (value >>  SHIFT)                & 7;
        auto cpu_mode  = (value >> (SHIFT+MODE_OFFSET)) & MODE_BIT_MASK; 
        val_p->set_arg(arg, reg_num | (cpu_mode << 3));
    }
};

// if MODE_OFFSET is three, just generic with N bits
template <unsigned SHIFT, unsigned WORD, unsigned BITS>
struct fmt_reg_mode<SHIFT, WORD, 3, BITS> : fmt_generic<SHIFT, 3 + BITS, WORD> {};

// insert a "branch displacement"
struct fmt_displacement : m68k_mcode_t::fmt_t::fmt_impl
{
    using val_t = m68k_mcode_t::val_t;
   
    // branch `machine code` insertions handled by `emit_reloc`
    void emit_reloc(core::emit_base& base, uint16_t *op, m68k_arg_t& arg, val_t const *val_p) const override
    {
        // calculate size here
        switch (arg.mode())
        {
        default:
            std::cout << "emit_relocation: bad arg: " << arg << ", mode = " << +arg.mode() << std::endl;
           break;
           throw std::logic_error{"invalid fmt_displacement mode"};

        case MODE_BRANCH_BYTE:
        {
            // byte displacement stored in LSBs. Use reloc mechanism

            std::cout << "fmt_displacement: branch byte" << std::endl;
            // displacement is from pc + 2, 1 byte offset from base machine code word
            // XXX offset??
            // reloc is 8-bits & pc-relative
            elf::kas_reloc r { elf::K_REL_ADD, 8, true }; 
            
            // reloc is from end of machine code, with 1 byte offset
            base << core::emit_reloc(r, -2, 1) << arg.expr;
            break;
        }
        case MODE_BRANCH_WORD:
            std::cout << "fmt_displacement: branch word" << std::endl;
            break;                  // zero LSBs indicate word offset will follow
        case MODE_BRANCH_LONG:
            std::cout << "fmt_displacement: branch long" << std::endl;
            *op |= 0xff;            // ones LSBs indicate long offset will follow
            break;
        }
    }
};


// Format PAIRs: Must specify offsets. Default is 3-bits in WORD1
// NB: if single register is specified for `PAIR` it is duplicated in both positions
template <unsigned SHIFT_0, unsigned SHIFT_1, unsigned BITS = 3, unsigned WORD_0 = 1, unsigned WORD_1 = WORD_0>
struct fmt_reg_pair : m68k_mcode_t::fmt_t::fmt_impl
{
    // assert anticipated cases
    static_assert(BITS == 3 || BITS == 4);
    
    using val_t = m68k_mcode_t::val_t;
    static constexpr auto BITS_MASK = (1 << BITS) - 1;
    static constexpr auto MASK_0 = (BITS_MASK << SHIFT_0);
    static constexpr auto MASK_1 = (BITS_MASK << SHIFT_1);

    bool insert(uint16_t* op, m68k_arg_t& arg, val_t const *val_p) const override
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
    
    void extract(uint16_t const* op, m68k_arg_t& arg, val_t const *val_p) const override
    {
        // XXX if pair is resolved to same reg twice, should disassembler report
        // XXX `REG` or `REG:REG`. I belive all insns assemble the same with
        // XXX REG & REG:REG.
        auto get_reg = [](unsigned n) -> m68k_reg_t const&
            {
                auto mode = (n & 8) ? MODE_ADDR_REG : MODE_DATA_REG;
                return m68k_reg_t::find(mode, n & 7);
            };
            
        auto reg1 = (op[WORD_0] & MASK_0) >> SHIFT_0;
        auto reg2 = (op[WORD_1] & MASK_1) >> SHIFT_1;
        
        // `deserializer` overrides mode. may be overwritten to DATA_REG or ADDR_REG
        // set `reg_num` for these cases. No-op for MODE_PAIR
        arg.reg_num = reg1 & 7;
        
        // generate `MODE_PAIR` arg
        arg.reg_p = &get_reg(reg1);
        arg.outer = get_reg(reg2);
        arg.set_mode(MODE_PAIR);
    }
};

// coldfire MAC (especially eMAC) throws bits all over the place
// Three register bits are shifted by SHIFT and inserted.
// The fourth general register bit (data/addr) is shifted by SHIFT+OFFSET & inserted.
// The "subword" bit is shifted by by SUB_BIT (if >= 0) & inserted in SUB_WORD

// data format from validator: 4 LSBs: general register #, SUB_BIT << 4
template <unsigned SHIFT, int B4_OFFSET, int SUB_BIT = -1
                        , unsigned WORD = 0, unsigned SUB_WORD = 1>
struct fmt_subreg : m68k_mcode_t::fmt_t::fmt_impl
{
    using val_t = m68k_mcode_t::val_t;

    bool insert(uint16_t* op, m68k_arg_t& arg, val_t const *val_p) const override
    {
        // validator returns 7 bits: subreg + mode + reg
        // NB: 4 lsbs happen to be general register #
        auto value = val_p->get_value(arg);

        // mask out current register bits
        op[WORD] &=~ (7 << SHIFT) | (1 << (SHIFT + B4_OFFSET));
        if constexpr (B4_OFFSET == 3)
            op[WORD] |=  (value & 15) << SHIFT;
        else
        {
            op[WORD] |= (value & 7) << SHIFT;
            op[WORD] |= (value & 8) << (SHIFT + B4_OFFSET - 3);
        }

        if constexpr (SUB_BIT >= 0)
        {
            op[SUB_WORD] &=~ (1 << SUB_BIT);
            op[SUB_WORD] |=  !!(VAL_SUBWORD_UPPER & value) << SUB_BIT;
        }

        return true;
    }
    
    void extract(uint16_t const* op, m68k_arg_t& arg, val_t const *val_p) const override
    {
        // accumulate value
        uint16_t value{};

        // set subword value if required
        if constexpr (SUB_BIT >= 0)
            if (op[SUB_WORD] & (1 << SUB_BIT))
                value = VAL_SUBWORD_UPPER;
        
        // get general register bits
        // NB: if B4_OFFSET is 3, then just 4-bit value
        if constexpr (B4_OFFSET == 3)
            value |= 15 & (op[WORD] >> SHIFT);
        else
        {
            auto code = op[WORD];
            value |= 7 & (code >> SHIFT);
            value |= 8 & (code >> (SHIFT + B4_OFFSET - 3));
        }
        val_p->set_arg(arg, value);
    }
};

// emac accN stored: (optionally complemented) LSB in word 0, bit 7; MSB in word 1, bit 5
// think like a compiler when bit-twiddling...
template <bool INVERT_LSB = false>
struct fmt_emac_an : m68k_mcode_t::fmt_t::fmt_impl
{
    using val_t = m68k_mcode_t::val_t;

    bool insert(uint16_t* op, m68k_arg_t& arg, val_t const *val_p) const override
    {
        // validator returns 2 bits for ACC0->ACC3
        auto value = val_p->get_value(arg);

        // store MSB
        op[1] &=~ (1 << 5);
        op[1] |=  (value & 2) << (5 - 1);
        
        // store LSB
        op[0] &=~ (1 << 7);
        op[0] |= (!(value & 1) ^ !INVERT_LSB) << 7;
        return true;
    }
    
    void extract(uint16_t const* op, m68k_arg_t& arg, val_t const *val_p) const override
    {
        // get MSB
        auto value =  (op[1] >> (5 - 1)) & 2;

        // get LSB
        if (!(op[0] & (1 << 7)) == INVERT_LSB)
            ++value;

        val_p->set_arg(arg, value);
    }
};


}
#endif


