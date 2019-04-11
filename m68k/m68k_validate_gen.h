#ifndef KAS_M68K_VALIDATE_GEN_H
#define KAS_M68K_VALIDATE_GEN_H

/******************************************************************************
*
* Instruction argument validation.
*
* There are four types of argument validation supported:
*
* 1) access mode validation: These modes are described in the
*    M68K Programmers Reference Manual (eg: Table 2-4 in document
*    M680000PM/AD) and used throughout opcode descriptions.
*
* 2) register class: based on class enum (eg: RC_DATA or RC_CTRL)
*
* 3) single register: base on class enum & value
*    (eg: RC_CTRL:0x800 for `USP`)
*
* 4) arbitrary function: these can be used to, for example, validate
*    the many constant ranges allowed by various instructions. Examples
*    include `MOVEQ` (signed 8 bits), `ADDQ` (1-8 inclusive), etc.
*    Function may also specify argument size calculation functions.
*
* For each type of validation, support two methods:
*      - fits_result ok(arg&, info&, fits&) : test argument against validation
*      - op_size_t   size(arg&, info&, fits&, size_p*) : bytes required by arg
*
*****************************************************************************/

//#include "m68k_insn_validate.h"
#include "m68k_mcode.h"
#include "target/tgt_validate.h"

namespace kas::m68k::opc
{

namespace 
{
using m68k_validate = typename m68k_mcode_t::val_t;

// the "range" validators all resolve to zero size
struct val_range : m68k_validate
{
    constexpr val_range(int32_t min, int32_t max, int8_t zero = 0)
                : min(min), max(max), zero(zero) {}

    fits_result ok(m68k_arg_t& arg, expr_fits const& fits) const override
    {
        // range is only for immediate args
        switch (arg.mode())
        {
            case MODE_IMMED:
            case MODE_IMMED_BYTE:
            case MODE_IMMED_WORD:
            case MODE_IMMED_LONG:
                if (auto p = arg.expr.get_fixed_p())
                {
                    // if zero is mapped, block it.
                    if (!*p && zero)
                        return fits.no;
                    return fits.fits(*p, min, max);
                }
                return fits.fits(arg.expr, min, max);
            default:
                return fits.no;
        }
    }

    unsigned get_value(m68k_arg_t& arg) const override
    {
        // calclulate value to insert in machine code
        auto p = arg.expr.get_fixed_p();
        auto n = p ? *p : 0;
        return n == zero ? 0 : n;
    }

    void set_arg(m68k_arg_t& arg, unsigned value) const override
    {
        // calculate expression value from machine code
        arg.expr = value ? value : zero;
        arg.set_mode(MODE_IMMED);
    }
    
    int32_t min, max;
    int8_t  zero;
};

struct val_dir_long : m68k_validate
{
    fits_result ok(m68k_arg_t& arg, expr_fits const& fits) const override
    {
        // allow addr_indir or addr_indr_displacement
        if (arg.mode() == MODE_DIRECT)
            arg.set_mode(MODE_DIRECT_LONG);

        if (arg.mode() == MODE_DIRECT_LONG)
            return fits.yes;

        return fits.no;
    }

    fits_result size(m68k_arg_t& arg, uint8_t sz, expr_fits const& fits
                                                    , op_size_t& op_size) const override
    {
        op_size += 4;
        return fits.yes;
        //return coldfire_limit(expr_fits::yes, size_p);
    }
};

struct val_direct_del : m68k_validate
{
    // special for branch to flag deletable branch
    fits_result ok(m68k_arg_t& arg, expr_fits const& fits) const override
    {
        // allow direct mode 
        static constexpr auto am = AM_DIRECT;

        if ((arg.am_bitset() & am) == am)
            return fits.yes;
        return fits.no;
    }

    // NB: this validator is reason we need size_p: whole insn can be deleted
    fits_result size(m68k_arg_t& arg, uint8_t sz, expr_fits const& fits
                                                    , op_size_t& op_size) const override
    {
        // set total size min to zero to flag branch can delete instruction
        op_size = { 0, 6 };     // branch with byte/word/long displacement
        return expr_fits::yes;   
    }
};

struct val_movep : m68k_validate
{
    fits_result ok(m68k_arg_t& arg, expr_fits const& fits) const override
    {
        switch (arg.mode())
        {
            case MODE_ADDR_INDIR:
                arg.expr = {};
                // FALLSTHRU
            case MODE_ADDR_DISP:
            case MODE_MOVEP:
                arg.set_mode(MODE_MOVEP);
                return fits.yes;
            default:
                return fits.no;
        }
    }
    fits_result size(m68k_arg_t& arg, uint8_t sz, expr_fits const& fits
                                                    , op_size_t& op_size) const override
    {
        op_size += 2;               // word displacement always present
        return expr_fits::yes;      // move_p not a coldfire insn
    }
};

struct val_pair : m68k_validate
{
    constexpr val_pair(bool addr_ok) : addr_ok(addr_ok) {}

    fits_result ok(m68k_arg_t& arg, expr_fits const& fits) const override
    {
        // check for pair of DATA or GENERAL registers
        if (arg.mode() != MODE_PAIR)
            return fits.no;

        // NB: make use of the fact the RC_DATA == 0 & RC_ADDR == 1
        auto max_reg_class = RC_DATA + addr_ok;

        // check first of pair
        auto rp = arg.expr.template get_p<m68k_reg_t>();
        if (!rp || rp->kind() > max_reg_class)
            return fits.no;

        // check second of pair
        rp = arg.outer.template get_p<m68k_reg_t>();
        if (!rp || rp->kind() > max_reg_class)
            return fits.no;

        return fits.yes;
    }        
   
    // val_pair is "paired" for `fmt_reg_pair` for insertion/deletion
    // interchange format: first "reg" is 4 LSBs. second is next 4 bits shifted 4.
    unsigned get_value(m68k_arg_t& arg) const override
    {
        // calclulate value to pass to `fmt_reg_pair`
        auto gen_reg_value = [](auto const& e)
            {
                auto p = e.template get_p<m68k_reg_t>();
                auto n = p->value();
                if (p->kind() == RC_ADDR)
                    n += 8;
                return n;
            };
        auto reg1 = gen_reg_value(arg.expr);
        auto reg2 = gen_reg_value(arg.outer);

        return reg1 | (reg2 << 4);
    }

    void set_arg(m68k_arg_t& arg, unsigned value) const override
    {
        auto calc_reg = [](auto value) -> m68k_reg_t
            {
                auto rc = value & 8 ? RC_ADDR : RC_DATA;
                uint16_t reg_num = value & 7;
                return { rc, reg_num };
            };

        // calculate expression value from machine code
        arg.expr  = calc_reg(value);
        arg.outer = calc_reg(value >> 4);
        arg.set_mode(MODE_PAIR);
    }
    
    const bool addr_ok{};
};

// REGSET Functions: Allow single register or regset of appropriate kind()
struct val_regset : m68k_validate
{
    constexpr val_regset(uint8_t kind = {}, bool rev = false)
                : kind{kind}, rev(rev) {}
    
    fits_result ok(m68k_arg_t& arg, expr_fits const& fits) const override
    {
        switch (arg.mode()) {
            case MODE_IMMED:
            case MODE_IMMED_LONG:
            case MODE_IMMED_WORD:
                return fits.yes;
            case MODE_REGSET:
                if (auto rs_p = arg.expr.template get_p<m68k_reg_set>())
                {
                    if (!kind && rs_p->kind() <= RC_ADDR)
                        return fits.yes;
                    if (rs_p->kind() == kind)
                        return fits.yes;
                }
                break;
            case MODE_DATA_REG:
            case MODE_ADDR_REG:
                if (!kind)
                    return fits.yes;
                break;
            case MODE_REG:
                if (arg.expr.template get_p<m68k_reg_t>()->kind() == kind)
                    return fits.yes;
                break;
            default:
                break;
        }
        return fits.no;
    }

    unsigned get_value(m68k_arg_t& arg) const override
    {
        // calclulate value to insert in machine code
        return 0;
    }

    void set_arg(m68k_arg_t& arg, unsigned value) const override
    {
        // calculate expression value from machine code
        arg.expr = value;
        arg.set_mode(MODE_IMMED);
    }
    
    // kind == 0 for general register
    uint8_t kind;
    bool    rev;
};

struct val_bitfield : m68k_validate
{
    fits_result ok(m68k_arg_t& arg, expr_fits const& fits) const override
    {
        auto bf_fits = [&](auto& e) -> fits_result
            {
                if (auto rp = e.template get_p<m68k_reg_t>())
                    return rp->kind() == RC_DATA ? fits.yes : fits.no;
                return fits.fits(e, 0, 31);
            };

        if (arg.mode() != MODE_BITFIELD)
            return fits.no;

        auto offset_fits = bf_fits(arg.expr);
        auto width_fits  = bf_fits(arg.outer);

        // both must fit for a yes.
        if (offset_fits == fits.no || width_fits == fits.no)
            return fits.no;

        // both either yes or maybe
        return offset_fits != fits.yes ? offset_fits : width_fits;
    }
    unsigned get_value(m68k_arg_t& arg) const override
    {
        // calclulate value to insert in machine code
        auto calc = [](auto& e) -> uint16_t
            {
                if (auto rp = e.template get_p<m68k_reg_t>())
                    return rp->value();
                if (auto p = e.get_fixed_p())
                    return *p & 0x1f;
                return 0;
            };

        auto offset = calc(arg.expr);
        auto width  = calc(arg.outer);
        return (offset << 6) | width;
    }

    void set_arg(m68k_arg_t& arg, unsigned value) const override
    {
        // calculate expression value from machine code
        auto get_expr = [](auto value) -> expr_t
            {
                if (value & 0x20)
                    return m68k_reg_t{ RC_DATA, value & 7 };
                return value & 0x1f;
            };

        arg.outer = get_expr(value);
        arg.expr  = get_expr(value >> 6);
        arg.set_mode(MODE_BITFIELD);
    }
};

struct val_subreg : m68k_validate
{
    fits_result ok(m68k_arg_t& arg, expr_fits const& fits) const override
    {
        // check for ... MAC registers
        if (arg.mode() > MODE_ADDR_REG)
            return fits.no;
        if (arg.reg_subword != REG_SUBWORD_FULL)
            return fits.yes;
        return fits.no;
    }
    
    unsigned get_value(m68k_arg_t& arg) const override
    {
        // calclulate value to insert in machine code
        auto value = arg.reg_num;
        if (arg.mode() == MODE_ADDR_REG)
            value += 8;
        if (arg.reg_subword == REG_SUBWORD_UPPER)
            value += 16;
        return value;
    }

    void set_arg(m68k_arg_t& arg, unsigned value) const override
    {
        // calculate expression value from machine code
        arg.reg_num     = value & 7;
        arg.reg_subword = value & 16 ? REG_SUBWORD_UPPER : REG_SUBWORD_LOWER;
        arg.set_mode(value & 8 ? MODE_ADDR_REG : MODE_DATA_REG);
    }
    

};

struct val_acc : m68k_validate
{
    fits_result ok(m68k_arg_t& arg, expr_fits const& fits) const override
    {
        auto rp = arg.expr.template get_p<m68k_reg_t>();
        if (!rp || rp->kind() != RC_CPU)
            return fits.no;

        if (rp->value() < REG_CPU_ACC0)
            return fits.no;

        if (rp->value() <= REG_CPU_ACC3)
            return fits.yes;

        return fits.no;
    }
    
    unsigned get_value(m68k_arg_t& arg) const override
    {
        // calclulate value to insert in machine code
        auto n = arg.expr.template get_p<m68k_reg_t>()->value();
        return n - REG_CPU_ACC0;
    }

    void set_arg(m68k_arg_t& arg, unsigned value) const override
    {
        // calculate expression value from machine code
        arg.expr = m68k_reg_t { RC_CPU, value + REG_CPU_ACC0 };
        arg.set_mode(MODE_REG);
    }
    
};

// Allow different MODES that M68K for the CF `BTST` insn only.
// Check that CF may disallow anyway, via 3-word limit
struct val_cf_bit_tst : m68k_validate
{
    fits_result ok(m68k_arg_t& arg, expr_fits const& fits) const override
    {
        // use AM bits: test is MEMORY w/o IMMED
        if (arg.am_bitset() & AM_IMMED)
            return fits.no;

        if (arg.am_bitset() & AM_MEMORY)
            return fits.yes;
        return fits.no;
    }
};

// Allow different MODES that M68K. 
// Check that CF may disallow anyway, via 3-word limit
struct val_cf_bit_static : m68k_validate
{
    fits_result ok(m68k_arg_t& arg, expr_fits const& fits) const override
    {
        // for coldfire BIT instructions on STATIC bit # cases.
        auto mode_norm = arg.mode_normalize();
        switch (arg.mode())
        {
            case MODE_ADDR_INDIR:
            case MODE_POST_INCR:
            case MODE_PRE_DECR:
            case MODE_ADDR_DISP:
                return fits.yes;
            default:
                return fits.no;
        }
    }
};

}   // unnamed namespace


#define VAL_GEN(NAME, ...) using NAME = _val_gen<KAS_STRING(#NAME), __VA_ARGS__>;

template <typename N, typename T, int...Ts>
using _val_gen = list<N, T, int_<Ts>...>;

VAL_GEN (Q_IMMED,    val_range, -128, 127);     // 8 bits signed (moveq)
VAL_GEN (Q_MATH,     val_range, 1,   8, 8);     // allow 1-8 inclusive
VAL_GEN (Q_3BITS,    val_range, 0,   7);
VAL_GEN (Q_4BITS,    val_range, 0,  15);
VAL_GEN (Q_7BITS,    val_range, 0, 127);
VAL_GEN (Q_8BITS,    val_range, 0, 255);
VAL_GEN (Z_IMMED,    val_range, 0,   0);
VAL_GEN (Q_MOV3Q,    val_range, -1,  7, -1);    // map -1 to value zero

// XXX is this just immed?
VAL_GEN (BIT_IMMED,  val_range, -0x7fff'ffff, 0x7fff'ffff);

VAL_GEN (ADDR_DISP,  val_range, 0, 0);   // mode == ADDR_DISP ? XXX convert to AM
VAL_GEN (ADDR_INDIR, val_range, 0, 1);   // mode == ADDR_INDIR ? XXX convert to AM
VAL_GEN (INDIR_MASK, val_range, 0, 2);   // mode in (INDIR, INCR, DECR, DISP)

VAL_GEN (PAIR,       val_pair, 0);       // data pair
VAL_GEN (GEN_PAIR,   val_pair, 1);       // gen-reg pair

VAL_GEN (DIR_LONG,   val_dir_long);      // DIRECT_LONG                  w/ size fn
VAL_GEN (DIRECT_DEL, val_direct_del);    // DELETABLE branch             w/ size fn

VAL_GEN (MOVEP,      val_movep);         // indir or disp.

VAL_GEN (BITFIELD,   val_bitfield);      // BITFIELD test

VAL_GEN (REGSET,     val_regset, RC_DATA);      // REGSET test
VAL_GEN (REGSET_REV, val_regset, RC_DATA, true);
VAL_GEN (FP_REGSET,  val_regset, RC_FLOAT);
VAL_GEN (FC_REGSET,  val_regset, RC_FCTRL);

// coldfire BIT insn validators
VAL_GEN (CF_BIT_TST   , val_cf_bit_tst);     // MEM w/o IMMED (would limit3 knock this out?)
VAL_GEN (CF_BIT_STATIC, val_cf_bit_static);  // CTRL_ALTER w/o abs & index

// coldfire MAC validators
VAL_GEN (REG_UL,     val_subreg);            // test SUBWORD field
VAL_GEN (ACC_N,      val_acc);               // test range RC_CPU: ACC0 thru ACC3 inclusive

#undef VAL_GEN
}
#endif
