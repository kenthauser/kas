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
 * For each type of validation, support three methods:
 *      - const char* name()                 : return name of validation
 *      - fits_result ok(arg&, info&, fits&) : test argument against validation
 *      - op_size_t   size(arg&, info&, fits&, size_p*) : bytes required by arg
 *
 *
 *****************************************************************************
 *
 * Implementation
 *
 * 
 *
 *
 *

 *****************************************************************************/

#include "m68k_insn_validate.h"

namespace kas::m68k::opc
{

namespace 
{
    // the "range" validators all resolve to zero size
    struct val_range : m68k_validate
    {
        constexpr val_range(int32_t min, int32_t max) : min(min), max(max) {}
    
        fits_result ok(m68k_arg_t& arg, m68k_size_t, expr_fits const& fits) const override
        {
            // range is only for immediate args
            switch (arg.mode()())
            {
                case MODE_IMMED:
                case MODE_IMMED_BYTE:
                case MODE_IMMED_WORD:
                case MODE_IMMED_LONG:
                    return fits.fits(arg.expr, min, max);
                default:
                    return fits.no;
            }
        }

        int32_t min, max;
    };

    struct val_mov3q : m68k_validate
    {
        fits_result ok(m68k_arg_t& arg, m68k_size_t, expr_fits const& fits) const override
        {
            switch (arg.mode())
            {
                case MODE_IMMED:
                case MODE_IMMED_BYTE:
                case MODE_IMMED_WORD:
                case MODE_IMMED_LONG:
                    // disallow zero
                    if (fits.zero(arg.expr))
                        return fits.no;
                    return fits.fits(arg.expr, -1, 7);
                default:
                    return fits.no;
            }
        }
    };

    struct val_dir_long : m68k_validate
    {
        fits_result ok(m68k_arg_t& arg, m68k_size_t, expr_fits const& fits) const override
        {
            // allow addr_indir or addr_indr_displacement
            if (arg.mode() == MODE_DIRECT)
                arg.mode() = MODE_DIRECT_LONG;

            if (arg.mode() == MODE_DIRECT_LONG)
                return fits.yes;

            return fits.no;
        }

        fits_result size(m68k_arg_t&, m68k_size_t, expr_fits const&, op_size_t *size_p) const override
        {
            *size_p += 4;
            return coldfire_limit(expr_fits::yes, size_p);
        }
    };

    struct val_direct_del : m68k_validate
    {
        // special for branch to flag deletable branch
        fits_result ok(m68k_arg_t& arg, m68k_size_t, expr_fits const& fits) const override
        {
            // allow direct mode 
            static constexpr auto am = AM_DIRECT;

            if ((arg.am_bitset() & am) == am)
                return fits.yes;
            return fits.no;
        }

        // NB: this validator is reason we need size_p: whole insn can be deleted
        fits_result size(m68k_arg_t&, m68k_size_t, expr_fits const&, op_size_t *size_p) const override
        {
            // set total size min to zero to flag branch can delete instruction
            *size_p = { 0, 6 };     // branch with byte/word/long displacement
            return expr_fits::yes;   
        }
    };

    struct val_movep : m68k_validate
    {
        fits_result ok(m68k_arg_t& arg, m68k_size_t, expr_fits const& fits) const override
        {
            switch (arg.mode())
            {
                case MODE_ADDR_INDIR:
                    arg.expr = {};
                    // FALLSTHRU
                case MODE_ADDR_DISP:
                case MODE_MOVEP:
                    arg.mode() = MODE_MOVEP;
                    return fits.yes;
                default:
                    return fits.no;
            }
        }
        fits_result size(m68k_arg_t&, m68k_size_t, expr_fits const&, op_size_t *size_p) const override
        {
            *size_p += 2;               // word displacement always present
            return expr_fits::yes;      // move_p not a coldfire insn
        }
    };

    struct val_pair : m68k_validate
    {
        constexpr val_pair(bool addr_ok) : addr_ok(addr_ok) {}

        fits_result ok(m68k_arg_t& arg, m68k_size_t, expr_fits const& fits) const override
        {
            // check for pair of DATA or GENERAL registers
            if (arg.mode() != MODE_PAIR)
                return fits.no;

            // NB: make use of the fact the RC_DATA == 0 & RC_ADDR == 1
            auto max_reg_class = RC_DATA + addr_ok;

            // check first of pair
            auto rp = arg.expr.template get_p<m68k_reg>();
            if (!rp || rp->kind() > max_reg_class)
                return fits.no;

            // check second of pair
            rp = arg.outer.template get_p<m68k_reg>();
            if (!rp || rp->kind() > max_reg_class)
                return fits.no;

            return fits.yes;
        }        
        
        const bool addr_ok{};
    };

    // REGSET Functions: Allow single register or regset of appropriate kind()
    struct val_regset : m68k_validate
    {
        constexpr val_regset(int kind = {}) : kind{kind} {}
        
        fits_result ok(m68k_arg_t& arg, m68k_size_t, expr_fits const& fits) const override
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
                default:
                    break;
            }
            return fits.no;
        }

        // kind == 0 for general register
        int kind{};
    };

    struct val_bitfield : m68k_validate
    {
        fits_result ok(m68k_arg_t& arg, m68k_size_t sz, expr_fits const& fits) const override
        {
            auto bf_fits = [&](auto& e) -> fits_result
                {
                    if (auto rp = e.template get_p<m68k_reg>())
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
    };

    struct val_subreg : m68k_validate
    {
        fits_result ok(m68k_arg_t& arg, m68k_size_t, expr_fits const& fits) const override
        {
            // check for ... MAC registers
            if (arg.reg_subword != REG_SUBWORD_FULL)
                return fits.yes;
            return fits.no;
        }

    };

    struct val_acc : m68k_validate
    {
        fits_result ok(m68k_arg_t& arg, m68k_size_t, expr_fits const& fits) const override
        {
            auto rp = arg.template get_p<m68k_reg>();
            if (!rp || rp->kind() != RC_CPU)
                return fits.no;

            if (rp->value() < REG_CPU_ACC0)
                return fits.no;

            if (rp->value() <= REG_CPU_ACC3)
                return fits.yes;

            return fits.no;
        }
    };

    struct val_cf_bit_tst : m68k_validate
    {
        fits_result ok(m68k_arg_t& arg, m68k_size_t, expr_fits const& fits) const override
        {
            // use AM bits: test is MEMORY w/o IMMED
            if (arg.am_bitset() & AM_IMMED)
                return fits.no;

            if (arg.am_bitset() & AM_MEMORY)
                return fits.yes;
            return fits.no;
        }
    };

    struct val_cf_bit_static : m68k_validate
    {
        fits_result ok (m68k_arg_t& arg, m68k_size_t, expr_fits const& fits) const override
        {
            // for coldfire BIT instructions on STATIC bit # cases.
            auto mode_norm = arg.mode()_normalize();
            switch (mode_norm) {
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


#define X_VAL_GEN(N, ...) using N = _val_gen<KAS_STRING(#N), __VA_ARGS__>;

template <typename N, typename T, int...Ts>
using _val_gen = list<N, T, int_<Ts>...>;

X_VAL_GEN (Q_IMMED,    val_range, -128, 127);   // 8 bits signed (moveq)

X_VAL_GEN (Q_MATH,     val_range, 1,   8);       // allow 1-8 inclusive
X_VAL_GEN (Q_3BITS,    val_range, 0,   7);
X_VAL_GEN (Q_4BITS,    val_range, 0,  15);
X_VAL_GEN (Q_7BITS,    val_range, 0, 127);
X_VAL_GEN (Q_8BITS,    val_range, 0, 255);
X_VAL_GEN (Z_IMMED,    val_range, 0,   0);
X_VAL_GEN (Q_MOV3Q,    val_mov3q);

// XXX is this just immed?
X_VAL_GEN (BIT_IMMED,  val_range, -0x7fff'ffff, 0x7fff'ffff);

X_VAL_GEN (ADDR_DISP,  val_range, 0, 0);   // mode == ADDR_DISP ? XXX convert to AM
X_VAL_GEN (ADDR_INDIR, val_range, 0, 1);   // mode == ADDR_INDIR ? XXX convert to AM
X_VAL_GEN (INDIR_MASK, val_range, 0, 2);   // mode in (INDIR, INCR, DECR, DISP)

X_VAL_GEN (PAIR,       val_pair, 0);       // data pair
X_VAL_GEN (GEN_PAIR,   val_pair, 1);       // gen-reg pair

X_VAL_GEN (DIR_LONG,   val_dir_long);      // DIRECT_LONG                  w/ size fn
X_VAL_GEN (DIRECT_DEL, val_direct_del);    // DELETABLE branch             w/ size fn

X_VAL_GEN (MOVEP,      val_movep);         // indir or disp.

X_VAL_GEN (BITFIELD,   val_bitfield);      // BITFIELD test

X_VAL_GEN (REGSET,     val_regset);        // REGSET test
X_VAL_GEN (FP_REGSET,  val_regset, RC_FLOAT);
X_VAL_GEN (FC_REGSET,  val_regset, RC_FCTRL);

// coldfire BIT insn validators
X_VAL_GEN (CF_BIT_TST, val_cf_bit_tst);        // MEM w/o IMMED (would limit3 knock this out?)
X_VAL_GEN (CF_BIT_STATIC, val_cf_bit_static);  // CTRL_ALTER w/o abs & index

// coldfire MAC validators
X_VAL_GEN (REG_UL,     val_subreg);            // test SUBWORD field
X_VAL_GEN (ACC_N,      val_acc);               // test range RC_CPU: ACC0 thru ACC3 inclusive

}

#if 0
    inline bool register_size_fn(VALIDATE_PFS fn, SIZE_PFS size)
    {
        arg_validate_t::register_size_fn(fn, size);
        return true;
    }
#endif

#endif
