#ifndef M68K_M68K_FORMATS_IMPL_H
#define M68K_M68K_FORMATS_IMPL_H


// processor specific types to insert/extract values
// from machine code
//
// type interface: two static methods
//
// bool insert(mcode_size_t *op, arg_t& arg, val_t const *val_p)
// void extract(mcode_size_t const *op, arg_t *arg, val_t *val_p)
//
// in the above:
//  *op     points to first word of "machine code"
//  arg     is current argument being processed. 
//  *val_p  points to current argument validator
//
// insert returns `true` if argument completely stored in machine code


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
    // actual word mask
    static constexpr auto MASK      = (7 << SHIFT) | (7 << (SHIFT+MODE_OFFSET));

    // shifted word mask
    static constexpr auto MODE_MASK = ((1 << MODE_BITS) - 1) << 3;

    static bool insert(uint16_t* op, m68k_arg_t& arg, val_t const *val_p)
    {
        kas::expression::expr_fits fits;
        
        // validator return 6 bits: mode + reg
        auto value = val_p->get_value(arg);
        auto cpu_reg  = value & 7;
        auto cpu_mode = arg.mode_normalize() & MODE_MASK;

        value  = cpu_reg  <<  SHIFT;
        value |= cpu_mode << (SHIFT+MODE_OFFSET-3);
        
        op[WORD]  &= ~MASK;
        op[WORD]  |= value;

        return fits.zero(arg.expr) == fits.yes;
    }
    
    static void extract(uint16_t const* op, m68k_arg_t* arg, val_t const *val_p)
    {
        auto value     = op[WORD];
        auto reg_num   = (value >>  SHIFT)                & 7;
        auto cpu_mode  = (value >> (SHIFT+MODE_OFFSET-3)) & MODE_MASK; 
        val_p->set_arg(*arg, reg_num | cpu_mode);
    }
};

// if MODE_OFFSET is three, just generic with N bits
template <unsigned SHIFT, unsigned WORD, unsigned BITS>
struct fmt_reg_mode<SHIFT, WORD, 3, BITS> : fmt_generic<SHIFT, 3 + BITS, WORD> {};

// Format PAIRs: Must specify offsets. Default is 3-bits in WORD1
// Interface with validator: 4 LSBs are first general register; second general register is << 4
template <unsigned SHIFT_0, unsigned SHIFT_1, unsigned BITS = 3, unsigned WORD_0 = 1, unsigned WORD_1 = WORD_0>
struct fmt_reg_pair
{
    // assert anticipated cases
    static_assert(BITS == 3 || BITS == 4);
    
    using val_t = m68k_mcode_t::val_t;
    static constexpr auto BITS_MASK = (1 << BITS) - 1;
    static constexpr auto MASK_0 = (BITS_MASK << SHIFT_0);
    static constexpr auto MASK_1 = (BITS_MASK << SHIFT_1);

    static bool insert(uint16_t* op, m68k_arg_t& arg, val_t const *val_p)
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
        
        if constexpr (WORD_0 != WORD_1)
        {
            auto code  = op[WORD_0] &= ~MASK_0;
            op[WORD_0] = code | (get_reg(arg.expr)  << SHIFT_0);
                 code  = op[WORD_1] &= ~MASK_1;
            op[WORD_1] = code | (get_reg(arg.outer) << SHIFT_1);
        }
        else
        {
            auto code  = op[WORD_0] &= ~(MASK_0 | MASK_1);
                 code |= (get_reg(arg.expr) ) << SHIFT_0;
                 code |= (get_reg(arg.outer)) << SHIFT_1;
            op[WORD_0] = code;
        }
        
        return true;
    }
    
    static void extract(uint16_t const* op, m68k_arg_t* arg, val_t const *val_p)
    {
    }
};

// bitfields are completely regular: bottom 12-bits of word 1 in fixed format
struct fmt_bitfield
{
    using val_t = m68k_mcode_t::val_t;
    //static constexpr auto MASK = (7 << SHIFT) | (7 << (SHIFT+MODE_OFFSET));

    static bool insert(uint16_t* op, m68k_arg_t& arg, val_t const *val_p)
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
    
    static void extract(uint16_t const* op, m68k_arg_t* arg, val_t const *val_p)
    {
#if 0
        auto value = op[WORD];
        auto reg_num  = (7 << 0) & (value >> SHIFT);
        auto cpu_mode = (7 << 3) & (value >> (SHIFT+MODE_OFFSET-3));
        
        val_p->set_arg(*arg, reg_num | cpu_mode);
#endif
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

    static bool insert(uint16_t* op, m68k_arg_t& arg, val_t const *val_p)
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
    
    static void extract(uint16_t const* op, m68k_arg_t* arg, val_t const *val_p)
    {
#if 0
        auto value = op[WORD];
        auto reg_num  = (7 << 0) & (value >> SHIFT);
        auto cpu_mode = (7 << 3) & (value >> (SHIFT+MODE_OFFSET-3));
        
        val_p->set_arg(*arg, reg_num | cpu_mode);
#endif
    }

};

// emac accN stored: complement of LSB in word 0, bit 7; MSB in word 1, bit 5
template <bool INVERT_LSB = false>
struct fmt_emac_an
{
    using val_t = m68k_mcode_t::val_t;
    //static constexpr auto MASK = (7 << SHIFT) | (7 << (SHIFT+MODE_OFFSET));

    static bool insert(uint16_t* op, m68k_arg_t& arg, val_t const *val_p)
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
    
    static void extract(uint16_t const* op, m68k_arg_t* arg, val_t const *val_p)
    {
#if 0
        auto value = op[WORD];
        auto reg_num  = (7 << 0) & (value >> SHIFT);
        auto cpu_mode = (7 << 3) & (value >> (SHIFT+MODE_OFFSET-3));
        
        val_p->set_arg(*arg, reg_num | cpu_mode);
#endif
    }
};

#if 0
// 3-bit register only
// for extract, specify "cpu_mode" (or 0x100 for RC_FLOAT, etc)
template <int SHIFT, unsigned MODE, unsigned WORD = 0, unsigned BITS = 3>
struct fmt_reg
{
    static constexpr auto MASK = (1 << BITS) - 1;
    static bool insert(uint16_t* op, m68k_arg_t& arg)
    {
        kas::expression::expr_fits fits;

        // validator return 6 bits: mode + reg
        auto value = val_p->get_value(arg);
        auto cpu_reg  = value & (7 << 0);
        auto cpu_mode = value & (7 << 3);

        auto reg = arg.cpu_reg();
        if (MODE & 0x100)
        {
            // floating point register
            reg = arg.expr.get_p<m68k_reg_t>()->value();
        }
        auto old_word = op[WORD];
        op[WORD] &= ~(MASK << SHIFT);
        op[WORD] |= reg << SHIFT;
        // std::cout << std::hex;
        // std::cout << "fmt_reg: " << old_word << " -> " << op[WORD];
        // std::cout << " mask = " << MASK  << " shift = " << SHIFT << std::endl;
        return true;
    }

    static void extract(uint16_t const* op, m68k_arg_t* arg)
    {
        auto value = op[WORD];
        auto reg_num  = (7 << 0) & (value >> SHIFT);
        auto cpu_mode = (7 << 3) & (value >> (SHIFT+MODE_OFFSET-3));
        
        val_p->set_arg(*arg, reg_num | cpu_mode);
        
        
        arg->reg_num = MASK & op[WORD] >> SHIFT;

        if (MODE & 0x100)
        {
            // floating point register
            arg->expr = m68k_reg_t(MODE & 0xff, arg->reg_num);
            arg->set_mode(MODE_REG);
        }
        else 
            arg->set_mode(MODE);
    }
};
#endif
#if 0
// registers which don't insert value. primarily for disassembler
template <unsigned REG_NUM, unsigned REG_CLASS = RC_CPU>
struct fmt_reg_direct
{
    static bool insert(uint16_t *op, m68k_arg_t& arg)
             { return true; }

    static void extract(uint16_t const *op, m68k_arg_t* arg)
    {
        arg->mode = MODE_REG;
        arg->disp = m68k_reg(REG_CLASS, REG_NUM);
    }
};


// floating point registers only:
// on insert: duplicate value in two places
template <unsigned WORD, int SHIFT, int DUP>
struct fmt_flt_duplicate
{
    static bool insert(uint16_t* op, m68k_arg_t& arg)
        {
            // can probably use `arg.reg_num`
            auto reg = arg.disp.get_p<m68k_reg>()->value();
            op[WORD] |= reg << SHIFT;
            op[WORD] |= reg << DUP;
            return true;
        }

    static void extract(uint16_t const* op, m68k_arg_t* arg)
        {
            arg->reg_num = 7 & op[WORD] >> SHIFT;

            // floating point register
            arg->disp = m68k_reg(RC_FLOAT, arg->reg_num);
            arg->mode = MODE_REG;
        }
};

// immediate argument (inserted into opcode, not appended)
// specify "type" for extract "mapping"
// specify "DIR = 1" to reverse bits in REGSET

template <int SHIFT, unsigned N, unsigned WORD = 0, bool DIR = false, typename T = uint16_t>
struct fmt_immed
{
    static constexpr auto MASK = (1 << N) - 1;
    static bool insert(uint16_t* op, m68k_arg_t& arg)
        {
            uint32_t disp;

            switch (arg.mode) {
                case MODE_DATA_REG:
                case MODE_ADDR_REG:
                {
                    // No error checking -- because no error path...
                    // create a "temporary" m68k_reg_set
                    auto reg = m68k_reg(arg.mode, arg.reg_num);
                    m68k_reg_set rs(reg);
                    disp = rs.value(DIR);

                    std::cout << "fmt_immed: " << reg << " -> ";
                    rs.print(std::cout);
                    std::cout << " = " << std::hex << disp << std::endl;
                    
                    auto reg2 = m68k_reg(arg.mode, arg.reg_num + 2);
                    auto& rs2 = rs - reg2;
                    
                    std::cout << "fmt_immed: " << reg2 << " -> ";
                    rs2.print(std::cout);
                    std::cout << " = " << std::hex << rs2.value(DIR) << std::endl;
                   
                    auto& rs3 = reg - reg2;
                    std::cout << "fmt_immed: ";
                    rs2.print(std::cout);
                    std::cout << " = " << std::hex << rs2.value(DIR) << std::endl;
                    break;
                 }
                case MODE_REG:
                {
                    auto reg_p = arg.disp.get_p<m68k_reg>();
                    // check for movec
                    if (reg_p->kind() == RC_CTRL) {
                        arg.mode = MODE_REG_QUICK;
                        disp = reg_p->value();
                        break;
                    }

                    // create regset from single register
                    disp = m68k_reg_set(*reg_p).value(DIR);
                    break;
                }
                case MODE_REGSET:
                    disp = arg.disp.get_p<m68k_reg_set>()->value(DIR);
                    break;
                default:
                    disp = *arg.disp.get_fixed_p();
                    break;
            }

            // std::cout << std::hex;
            // std::cout << "immed_write: " << (disp & MASK);
            // std::cout << " mask = " << MASK << " shift = " << SHIFT << std::endl;

            op[WORD] |=  (disp & MASK) << SHIFT;
            if (arg.mode != MODE_REG_QUICK)
                arg.mode = MODE_IMMED_QUICK;

            // std::cout << std::hex;
            // std::cout << "immed_write: " << (disp & MASK);
            // std::cout << " mask = " << MASK << " shift = " << SHIFT;
            // std::cout << " -> " << op[WORD] << std::endl;
            return true;

        }

    static void extract(uint16_t const* op, m68k_arg_t* arg)
        {
            // extract immediate value
            T n = MASK & (op[WORD] >> SHIFT);
            if (arg->mode == MODE_REG_QUICK)
                arg->disp = m68k_reg(RC_CTRL, n);
            else
                arg->disp = static_cast<expression::e_fixed_t>(n);
            // std::cout << "immed_quick: " << op[WORD] << " [" << n << "] -> " << arg->disp << std::endl;
        }
};

// special for clr.[lwb], spelled move.[lwb] #0: set src arg to mode_immed_quick
struct fmt_zero
{
    static bool insert(uint16_t *op, m68k_arg_t& arg)
    {
        arg.mode = MODE_IMMED_QUICK;
        return true;
    }

    static void extract(uint16_t const* op, m68k_arg_t* arg)
    {
        arg->mode = MODE_IMMED_QUICK;
        arg->disp = (expression::e_fixed_t)0;
    }
};

// 020 multiply/divide pair is regular
struct fmt_mult_pair
{
    static bool insert(uint16_t* op, m68k_arg_t& arg)
    {
        // assume only DATA REG/REG_PAIR args passed
        // perform no error checking.
        int reg1, reg2;

        if (arg.mode == MODE_PAIR) {
            reg1 = arg.disp.template  get_p<m68k_reg>()->value();
            reg2 = arg.outer.template get_p<m68k_reg>()->value();
        } else {
            // Allow DATA_REG to be converted to pair
            reg1 = reg2 = arg.reg_num;
        }
        op[1] |= reg1 | (reg2 << 12);
        return true;
    }

    static void extract(uint16_t const* op, m68k_arg_t* arg)
    {
        // extract registers
        auto reg1 = 7 & op[1];
        auto reg2 = 7 & (op[1] >> 12);

        // special case: if registers are same convert to DATA_REG
        if (reg1 == reg2) {
            arg->reg_num = reg1;
            arg->mode = MODE_DATA_REG;
        } else {
            arg->disp = m68k_reg(RC_DATA, reg1);
            arg->outer = m68k_reg(RC_DATA, reg2);
            arg->mode = MODE_PAIR;
        }
    }
};

// CPU32 table lookup pair. Also used for CAS2 register operands
template <int SHIFT = 0>
struct fmt_tbl_pair
{
    static bool insert(uint16_t* op, m68k_arg_t& arg)
    {
        //std::cout << "insert tbl_pair: " << arg << std::endl;
        auto reg1 = arg.disp.template  get_p<m68k_reg>()->value();
        auto reg2 = arg.outer.template get_p<m68k_reg>()->value();

        //std::cout << "insert tbl_pair: " << reg1 << ", " << reg2 << std::endl;
        op[0] |= reg1 << SHIFT;
        op[1] |= reg2 << SHIFT;
        return true;
    }

    static void extract(uint16_t const* op, m68k_arg_t* arg)
    {
        // extract registers
        auto reg1 = 7 & (op[0] >> SHIFT);
        auto reg2 = 7 & (op[1] >> SHIFT);

        arg->disp  = m68k_reg(RC_DATA, reg1);
        arg->outer = m68k_reg(RC_DATA, reg2);
        arg->mode  = MODE_PAIR;
    }
};

// CAS2 memory pairs
struct fmt_cas2_mem_pair
{
    static bool insert(uint16_t* op, m68k_arg_t& arg)
    {
        //std::cout << "insert cas2 reg_pair: " << arg << std::endl;
        auto reg_p1 = arg.disp.template  get_p<m68k_reg>();
        auto reg_p2 = arg.outer.template get_p<m68k_reg>();
    
        // create 4-bit general register
        auto reg1 = reg_p1->value() + (reg_p1->kind() == RC_ADDR ? 8 : 0);
        auto reg2 = reg_p2->value() + (reg_p2->kind() == RC_ADDR ? 8 : 0);
        //std::cout << "insert tbl_pair: " << +reg1 << ", " << + reg2 << std::endl;

        op[0] |= reg1 << 12;
        op[1] |= reg2 << 12;
        return true;
    }

    static void extract(uint16_t const* op, m68k_arg_t* arg)
    {
        // extract registers
        auto reg1 = op[0] >> 12;
        auto reg2 = op[1] >> 12;

        // allow general registers
        arg->disp  = m68k_reg(reg1 & 8 ? RC_ADDR : RC_DATA, reg1 & 7);
        arg->outer = m68k_reg(reg2 & 8 ? RC_ADDR : RC_DATA, reg2 & 7);
        arg->mode = MODE_PAIR;
    }
};

// 020 bitfield offset/width is regular
struct fmt_bitfield
{
    static bool insert(uint16_t* op, m68k_arg_t& arg)
    {
        auto eval_bf = [](auto& e)
            {
                if (auto p = e.get_fixed_p())
                    return *p & 0x3f;
                auto rp = e.template get_p<m68k_reg>();
                return 0x20 | rp->value();
            };

        //std::cout << "insert bitfield: " << arg << std::endl;
        op[1] |= eval_bf(arg.disp) << 6;
        op[1] |= eval_bf(arg.outer);
        return true;
    }

    static void extract(uint16_t const* op, m68k_arg_t* arg)
    {
        auto bf_expr = [](auto n) -> expr_t
            {
                if (n & 0x20)
                    return m68k_reg(RC_DATA, n & 7);
                return n & 0x1f;
            };

        arg->disp  = bf_expr(op[1] >> 6);
        arg->outer = bf_expr(op[1]);
        arg->mode  = MODE_BITFIELD;
    }
};

// coldfire MAC & eMAC general registers with U/L
// NOTES:
//  4-bit register stored in four places:
//      word 0, lower 4 bits
//      word 0, 3 LSBs shifted 9, msb at word 0 bit 6
//      word 1, 4 MSBs
//      word 1, 4 LSBs
// subreg bits:
//      src arg = word 1, bit 6
//      dst arg = word 1, bit 7
//      other args not subreg.
// Extract note:
//      if word 1 bit 11 set, no subregs
template <unsigned SHIFT, unsigned WORD = 0, unsigned ARG_N = 0>
struct fmt_regul
{
    static constexpr uint16_t BITS = 4;
    static constexpr uint16_t MASK = (1 << BITS) - 1;
    static bool insert(uint16_t* op, m68k_arg_t& arg)
        {
            // this is the MSB
            auto cpu_mode = arg.cpu_mode();
            if (cpu_mode > 1)
                return false;

            // these are the three LSBs
            auto cpu_reg  = arg.cpu_reg();

            if constexpr (SHIFT == 9) {
                static_assert (WORD == 0);
                op[0] &=~ (7 << 9);
                op[0] |=  cpu_reg << 9;
                op[0] |=  cpu_mode << 6;
            } else {
                if (cpu_mode)
                    cpu_reg |= (1 << 3);

                op[WORD] &=~ (MASK << SHIFT);
                op[WORD] |=  cpu_reg << SHIFT;
            }

            if constexpr (ARG_N == 1)
                if (arg.reg_subword == REG_SUBWORD_UPPER)
                    op[1] |= 1 << 6;
            
            if constexpr (ARG_N == 2)
                if (arg.reg_subword == REG_SUBWORD_UPPER)
                    op[1] |= 1 << 7;
            
            return true;
        }

    static void extract(uint16_t const* op, m68k_arg_t* arg)
        {
#if 0
            arg->reg_num  = 7 & op[WORD] >> SHIFT;
            int cpu_mode  = 7 & op[WORD] >> (SHIFT + MODE_OFFSET);
            arg->mode = cpu_mode != 7 ? cpu_mode : 7 + arg->reg_num;
#endif
        }
};

// coldfire emac ACC_n multiple accumulators
template <int SHIFT, unsigned WORD = 0, unsigned INVERT_LSB = 0>
struct fmt_accn
{
    static constexpr uint16_t BITS = 2;
    static constexpr uint16_t MASK = (1 << BITS) - 1;
    static bool insert(uint16_t* op, m68k_arg_t& arg)
        {
            // get accumulator value. Assume well formed
            auto rp    = arg.disp.template get_p<m68k_reg>();
            auto value = rp->value() - REG_CPU_ACC0;

            // one format drops two bits over two words...
            // ...and can invert LSB
            if constexpr (SHIFT == 7) {
                // LSB in first word
                op[0] &=~ (1 << 7);
                op[0] |=  ((value ^ INVERT_LSB) & 1) << 7;
                // MSB in second word
                op[1] &=~ (1 << 4);
                op[1] |=  (value & 2) << (4 - 1);
            } else {
                // only split bits support Invert LSB
                static_assert (INVERT_LSB == 0);
                auto old_word = op[WORD];
                op[WORD] &=~ (MASK << SHIFT);
                op[WORD] |=  value << SHIFT;
            }
            // std::cout << std::hex;
            // std::cout << "fmt_reg: " << old_word << " -> " << op[WORD];
            // std::cout << " mask = " << MASK  << " shift = " << SHIFT << std::endl;
            return true;
        }

    static void extract(uint16_t const* op, m68k_arg_t* arg)
        {
            uint16_t acc_n;
            if constexpr (SHIFT == 7) {
                acc_n  = ((op[0] >> 7) ^ INVERT_LSB) & 1;
                acc_n |= (op[1] >> (4-1)) & 2;
            } else {
                acc_n   = MASK & op[WORD] >> SHIFT;
            }
            
            auto reg_num = acc_n + REG_CPU_ACC0;
            arg->disp = m68k_reg(RC_CPU, reg_num);
            arg->mode = MODE_REG;
        }
};

// type to map 0->8 for shift/quick math extracts
struct q_math
{
    using T = expression::e_fixed_t;

    // XXX why is actual ctor required?
    q_math(T value) : value(value) {}
    
    // map zero to 8
    operator T() const { return value == 0 ? 8 : value; }
    expression::e_fixed_t const value;
};

// type to map -1->7 range for mov3q insn
struct q_mov3q : q_math
{
    using q_math::q_math;
    
    // map zero to -1
    operator T() const { return value == 0 ? -1 : value; }
};
#endif

}
#endif


