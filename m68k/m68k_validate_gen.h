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

#include "m68k_mcode.h"
#include "target/tgt_validate.h"

namespace kas::m68k::opc
{

namespace 
{
struct val_dir_long : m68k_mcode_t::val_t
{
    fits_result ok(m68k_arg_t& arg, m68k_stmt_info_t const& info, expr_fits const& fits) const override
    {
        switch (arg.mode())
        {
            default:
                break;

            case MODE_DIRECT:
            case MODE_DIRECT_LONG:
                return fits.yes;
        }
        return fits.no;
    }

    fits_result size(m68k_arg_t& arg, m68k_stmt_info_t const& info, expr_fits const& fits
                                                    , op_size_t& op_size) const override
    {
        op_size += 4;
        return fits.yes;
        //return coldfire_limit(expr_fits::yes, size_p);
    }
};

struct val_branch : m68k_mcode_t::val_t
{
    // special for branch. Allow deletable branch
    constexpr val_branch(bool can_delete = {}) : can_delete(can_delete) {}
    fits_result ok(m68k_arg_t& arg, m68k_stmt_info_t const& info, expr_fits const& fits) const override
    {
        // allow direct mode 
        static constexpr auto am = AM_DIRECT;

        if ((arg.am_bitset() & am) == am)
            return fits.yes;
        return fits.no;
    }

    // NB: this validator is reason we need size_p: whole insn can be deleted
    fits_result size(m68k_arg_t& arg, m68k_stmt_info_t const& info, expr_fits const& fits
                                                    , op_size_t& op_size) const override
    {
        op_size.max = 6;            // not on 68000

        // test if can delete. Slightly different so as to be sure
        if (can_delete && !fits.seen_this_pass(arg.expr))
            op_size.min = 0;
        else
            op_size.min = 2;
            
        // the following shift works for zero, two, four.
        // six will fail at `while` loop
        int disp_size = op_size.min >> 1;
        int mode = MODE_BRANCH_BYTE;
        while (op_size.min < op_size.max)
        {
            std::cout << "\nval_branch: fits: " << arg.expr << " sz = " << +disp_size << std::endl;
            // if attempting to delete, must use "displacment" of zero. Else sizeof(mcode_op_t)
            switch (fits.disp_sz(disp_size, arg.expr, op_size.min ? 2 : 0))
            {
                case expr_fits::no:
                    if (op_size.min)
                        ++mode;     // set next mode
                    break;          // try next value
                case expr_fits::yes:
                    arg.set_mode(mode);
                    op_size.max = op_size.min;
                    std::cout << "val_branch:yes: arg = " << arg << std::endl;
                    return expr_fits::yes;
                default:
                    std::cout << "val_branch:maybe: mode = " << std::dec << +mode << std::endl;
                    return expr_fits::maybe;
            }
            op_size.min += 2;
            ++disp_size;
        }

        // iff 68000, fits::no
        arg.set_mode(mode);
        std::cout << "val_branch:default: arg = " << arg << std::endl;
        return expr_fits::yes;   
    }

    bool can_delete;
};

struct val_movep : m68k_mcode_t::val_t
{
    fits_result ok(m68k_arg_t& arg, m68k_stmt_info_t const& info, expr_fits const& fits) const override
    {
        switch (arg.mode())
        {
            case MODE_ADDR_INDIR:
            case MODE_ADDR_DISP:
            case MODE_MOVEP:
                return fits.yes;
            default:
                return fits.no;
        }
    }
    fits_result size(m68k_arg_t& arg, m68k_stmt_info_t const& info, expr_fits const& fits
                                                    , op_size_t& op_size) const override
    {
        arg.set_mode(MODE_MOVEP);
        op_size += 2;               // word displacement always present
        return expr_fits::yes;      // move_p not a coldfire insn
    }
};

struct val_pair : m68k_mcode_t::val_t
{
    constexpr val_pair(bool addr_ok) : addr_ok(addr_ok) {}

    fits_result ok(m68k_arg_t& arg, m68k_stmt_info_t const& info, expr_fits const& fits) const override
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
struct val_regset : m68k_mcode_t::val_t
{
    constexpr val_regset(uint8_t kind = {}, bool rev = false)
                : kind{kind}, rev(rev) {}
    
    fits_result ok(m68k_arg_t& arg, m68k_stmt_info_t const& info, expr_fits const& fits) const override
    {
        switch (arg.mode()) {
            case MODE_IMMEDIATE:
            case MODE_IMMED_QUICK:
                // XXX test for fixed in range
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
        switch (arg.mode())
        {
            case MODE_IMMEDIATE:
                arg.set_mode(MODE_IMMED_QUICK);
                // FALLSTHRU
            case MODE_IMMED_QUICK:
                if (auto p = arg.expr.get_fixed_p())
                    return *p;
                return 0;
            case MODE_DATA_REG:
            case MODE_ADDR_REG:
            case MODE_REG:
            case MODE_REGSET:
                return 0;
            default:
            // calclulate value to insert in machine code
                return 0;
        }
    }

    void set_arg(m68k_arg_t& arg, unsigned value) const override
    {
        // calculate expression value from machine code
        arg.expr = value;
        arg.set_mode(MODE_IMMED_QUICK);
    }
    
    // kind == 0 for general register
    uint8_t kind;
    bool    rev;
};

struct val_bitfield : m68k_mcode_t::val_t
{
    // `is_register` is MSB of both offset & width fields
    static constexpr auto BF_FIELD_SIZE = 6;
    static constexpr auto BF_REG_BIT    = (1 << (BF_FIELD_SIZE - 1));
    static constexpr auto BF_MASK       = BF_REG_BIT - 1;

    fits_result ok(m68k_arg_t& arg, m68k_stmt_info_t const& info, expr_fits const& fits) const override
    {
        auto bf_fits = [&](auto& e) -> fits_result
            {
                if (auto rp = e.template get_p<m68k_reg_t>())
                    return rp->kind() == RC_DATA ? fits.yes : fits.no;
                return fits.fits(e, 0, BF_MASK);
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
                    return rp->value() | BF_REG_BIT;
                if (auto p = e.get_fixed_p())
                    return *p & BF_MASK;

                // XXX needs to `throw` if not fixed. no relocation available.
                // XXX possibly require constant expression in `OK`
                return 0;
            };

        auto offset = calc(arg.expr);
        auto width  = calc(arg.outer);
        return (offset << BF_FIELD_SIZE) | width;
    }

    void set_arg(m68k_arg_t& arg, unsigned value) const override
    {
        // calculate expression value from machine code
        auto get_expr = [](auto value) -> expr_t
            {
                if (value & BF_REG_BIT)
                    return m68k_reg_t{ RC_DATA, value & 7 };
                return value & BF_MASK;
            };

        arg.outer = get_expr(value);
        arg.expr  = get_expr(value >> BF_FIELD_SIZE);
        arg.set_mode(MODE_BITFIELD);
    }
};

struct val_subreg : m68k_mcode_t::val_t
{
    fits_result ok(m68k_arg_t& arg, m68k_stmt_info_t const& info, expr_fits const& fits) const override
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

struct val_acc : m68k_mcode_t::val_t
{
    fits_result ok(m68k_arg_t& arg, m68k_stmt_info_t const& info, expr_fits const& fits) const override
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
struct val_cf_bit_tst : m68k_mcode_t::val_t
{
    fits_result ok(m68k_arg_t& arg, m68k_stmt_info_t const& info, expr_fits const& fits) const override
    {
        // use AM bits: test is MEMORY w/o IMMED
        if (arg.am_bitset() & AM_IMMED)
            return fits.no;

        if (arg.am_bitset() & AM_MEMORY)
            return fits.yes;
        return fits.no;
    }
};

// Allow different MODES than M68K. 
// Check that CF may disallow anyway, via 3-word limit
struct val_cf_bit_static : m68k_mcode_t::val_t
{
    fits_result ok(m68k_arg_t& arg, m68k_stmt_info_t const& info, expr_fits const& fits) const override
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

// specialize generic validators
using val_range   = tgt::opc::tgt_val_range<m68k_mcode_t>;
template <typename T>
using val_range_t = tgt::opc::tgt_val_range_t<m68k_mcode_t, T>;

// QUICK validators. emit size is zero 
VAL_GEN (Q_MATH,     val_range, 1,   8, 8);     // allow 1-8 inclusive
VAL_GEN (Q_3BITS,    val_range, 0,   7);
VAL_GEN (Q_4BITS,    val_range, 0,  15);
VAL_GEN (Q_7BITS,    val_range, 0, 127);
VAL_GEN (Q_8BITS,    val_range, 0, 255);
VAL_GEN (Z_IMMED,    val_range, 0,   0);
VAL_GEN (Q_MOV3Q,    val_range, -1,  7, -1);    // map -1 to value zero

// immediate QUICK validators using T
VAL_GEN (Q_IMMED,    val_range_t<int8_t>);      // 8 bits signed (moveq)
VAL_GEN (Q_IMMED16,  val_range_t<int16_t>);     // 16 bits signed
VAL_GEN (BIT_IMMED,  val_range_t<int32_t>);     // 32 bits signed

// XXX 
VAL_GEN (ADDR_DISP,  val_range, 0, 0);   // mode == ADDR_DISP ? XXX convert to AM
VAL_GEN (ADDR_INDIR, val_range, 0, 1);   // mode == ADDR_INDIR ? XXX convert to AM

// XXX 
VAL_GEN (INDIR_MASK, val_range, 0, 2);   // mode in (INDIR, INCR, DECR, DISP)

VAL_GEN (PAIR,       val_pair, 0);       // data pair
VAL_GEN (GEN_PAIR,   val_pair, 1);       // gen-reg pair

VAL_GEN (BRANCH   ,  val_branch);       // BRANCH (displacement)
VAL_GEN (BRANCH_DEL, val_branch, 1);    // DELETABLE branch

VAL_GEN (MOVEP,      val_movep);         // indir or disp.

VAL_GEN (DIR_LONG   , val_dir_long);    // special for move16

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
