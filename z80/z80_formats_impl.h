#ifndef Z80_Z80_FORMATS_IMPL_H
#define Z80_Z80_FORMATS_IMPL_H

// 1. Remove `index` infrastructure
// 2. Split into virtual functions, "workers" and combiners
// 3. Add in `opc&` stuff

#include "z80_formats_type.h"

namespace kas::z80::opc
{
// tinker-toy functions to put args into various places...
// always paired: insert & extract

// 3-bit register only
template <int SHIFT, unsigned MODE, unsigned WORD = 0, unsigned BITS = 3>
struct fmt_reg
{
    static constexpr auto MASK = (1 << BITS) - 1;
    static bool insert(uint16_t* op, z80_arg_t& arg)
        {
            auto reg = arg.reg.value();
            
            auto old_word = op[WORD];
            op[WORD] &= ~(MASK << SHIFT);
            op[WORD] |= reg << SHIFT;
            // std::cout << std::hex;
            // std::cout << "fmt_reg: " << old_word << " -> " << op[WORD];
            // std::cout << " mask = " << MASK  << " shift = " << SHIFT << std::endl;
            return true;
        }

    static void extract(uint16_t const* op, z80_arg_t* arg)
        {
            auto reg_num = MASK & op[WORD] >> SHIFT;

            arg->mode = MODE;
        }
};

// registers which don't insert value. primarily for disassembler
template <unsigned REG_NUM, unsigned REG_CLASS = RC_GEN>
struct fmt_reg_direct
{
    static bool insert(uint16_t *op, z80_arg_t& arg)
             { return true; }

    static void extract(uint16_t const *op, z80_arg_t* arg)
    {
        arg->mode = MODE_REG;
        arg->reg  = z80_reg_t(REG_CLASS, REG_NUM);
    }
};

#if 0
// immediate argument (inserted into opcode, not appended)
// specify "type" for extract "mapping"
// specify "DIR = 1" to reverse bits in REGSET

template <int SHIFT, unsigned N, unsigned WORD = 0, bool DIR = false, typename T = uint16_t>
struct fmt_immed
{
    static constexpr auto MASK = (1 << N) - 1;
    static bool insert(uint16_t* op, z80_arg_t& arg)
        {
            uint32_t disp;

            switch (arg.mode) {
                case MODE_DATA_REG:
                case MODE_ADDR_REG:
                {
                    // No error checking -- because no error path...
                    // create a "temporary" z80_reg_set
                    auto reg = z80_reg(arg.mode, arg.reg_num);
                    z80_reg_set rs(reg);
                    disp = rs.value(DIR);

                    std::cout << "fmt_immed: " << reg << " -> ";
                    rs.print(std::cout);
                    std::cout << " = " << std::hex << disp << std::endl;
                    
                    auto reg2 = z80_reg(arg.mode, arg.reg_num + 2);
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
:                case MODE_REG:
                {
                    auto reg_p = arg.disp.get_p<z80_reg>();
                    // check for movec
                    if (reg_p->kind() == RC_CTRL) {
                        arg.mode = MODE_REG_QUICK;
                        disp = reg_p->value();
                        break;
                    }

                    // create regset from single register
                    disp = z80_reg_set(*reg_p).value(DIR);
                    break;
                }
                case MODE_REGSET:
                    disp = arg.disp.get_p<z80_reg_set>()->value(DIR);
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

    static void extract(uint16_t const* op, z80_arg_t* arg)
        {
            // extract immediate value
            T n = MASK & (op[WORD] >> SHIFT);
            if (arg->mode == MODE_REG_QUICK)
                arg->disp = z80_reg(RC_CTRL, n);
            else
                arg->disp = static_cast<expression::e_fixed_t>(n);
            // std::cout << "immed_quick: " << op[WORD] << " [" << n << "] -> " << arg->disp << std::endl;
        }
};
#endif

#if 0
// 020 bitfield offset/width is regular
struct fmt_bitfield
{
    static bool insert(uint16_t* op, z80_arg_t& arg)
    {
        auto eval_bf = [](auto& e)
            {
                if (auto p = e.get_fixed_p())
                    return *p & 0x3f;
                auto rp = e.template get_p<z80_reg>();
                return 0x20 | rp->value();
            };

        //std::cout << "insert bitfield: " << arg << std::endl;
        op[1] |= eval_bf(arg.disp) << 6;
        op[1] |= eval_bf(arg.outer);
        return true;
    }

    static void extract(uint16_t const* op, z80_arg_t* arg)
    {
        auto bf_expr = [](auto n) -> expr_t
            {
                if (n & 0x20)
                    return z80_reg(RC_DATA, n & 7);
                return n & 0x1f;
            };

        arg->disp  = bf_expr(op[1] >> 6);
        arg->outer = bf_expr(op[1]);
        arg->mode  = MODE_BITFIELD;
    }
};
// type to map 0->8 for shift/quick math extracts
struct q_math
{
    using T = expression::e_fixed_t;

    // XXX why is actual ctor required?
    constexpr q_math(T value) : value(value) {}
    
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


