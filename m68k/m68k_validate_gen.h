#ifndef KAS_M68K_M68K_VALIDATE_GEN_H
#define KAS_M68K_M68K_VALIDATE_GEN_H

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
#include "m68k_validate_reg.h"      // use AM_*
#include "target/tgt_validate_generic.h"
#include "target/tgt_validate_branch.h"

namespace kas::m68k::opc
{


struct val_indirect : m68k_mcode_t::val_t
{
    constexpr val_indirect(bool allow_disp = {}) : allow_disp(allow_disp) {};
    
    fits_result ok(m68k_arg_t& arg, expr_fits const& fits) const override
    {
        auto mode = arg.mode();
        
        switch (arg.mode())
        {
            default:
                break;

            case MODE_ADDR_DISP:
                if (!allow_disp)
                    break;
            // FALLSTHRU
            case MODE_ADDR_INDIR:
                return fits.yes;
        }
        return fits.no;
    }

    bool allow_disp;
};


struct val_dir_long : m68k_mcode_t::val_t
{
    fits_result ok(m68k_arg_t& arg, expr_fits const& fits) const override
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

    fits_result size(m68k_arg_t& arg, uint8_t sz
                   , expr_fits const& fits, op_size_t& op_size) const override
    {
        op_size += 4;
        return fits.yes;
    }
};

struct val_branch : tgt::opc::tgt_val_branch<m68k_mcode_t>
{
    using base_t::base_t; 
    
    // override `tgt_val_branch` methods
    fits_result ok(m68k_arg_t& arg, expr_fits const& fits) const override
    {
        // allow direct mode 
        static constexpr auto am = AM_DIRECT;

        if ((arg.am_bitset() & am) == am)
            return fits.yes;
        return fits.no;
    }

    // override `tgt_val_branch` method
    constexpr uint8_t max(m68k_mcode_t const& mc) const
    {
        if (!hw::cpu_defs[hw::branch_long{}])
            return 2;       // allow branch_word
        // XXX return base_t::max(mc);
        return 4;
    }
};

using val_branch_del = tgt::opc::tgt_val_branch_del<m68k_mcode_t>;

// NO_PC disallows "pc indirect". Designed for `branch` instructions
// where there is always a better `branch` insn
struct val_control_nodir : val_am
{
    // initialize as "CONTROL"
    constexpr val_control_nodir() : val_am(AM_CTRL) {}

    fits_result ok(m68k_arg_t& arg, expr_fits const& fits) const override
    {
        if (arg.mode() == MODE_DIRECT)
            return fits.no;
        return val_am::ok(arg, fits);
    }
};

struct val_movep : m68k_mcode_t::val_t
{
    fits_result ok(m68k_arg_t& arg, expr_fits const& fits) const override
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
    fits_result size(m68k_arg_t& arg, uint8_t sz
                   , expr_fits const& fits, op_size_t& op_size) const override
    {
        arg.set_mode(MODE_MOVEP);
        op_size += 2;               // word displacement always present
        return expr_fits::yes;      // move_p not a coldfire insn
    }

    unsigned get_value(m68k_arg_t& arg) const override
    {
        return arg.cpu_reg();
    }

    void set_arg(m68k_arg_t& arg, unsigned value) const override
    {
        arg.reg_num = value;
    }

    bool has_data(arg_t& arg) const override { return true; }
};

struct val_pair : m68k_mcode_t::val_t
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
        if (!arg.reg_p || arg.reg_p->kind() > max_reg_class)
            return fits.no;

        // check second of pair
        auto rp = arg.outer.template get_p<m68k_reg_t>();
        if (!rp || rp->kind() > max_reg_class)
            return fits.no;

        return fits.yes;
    }        
   
    // val_pair is "paired" for `fmt_reg_pair` for insertion/deletion
    // interchange format: first "reg" is 4 LSBs. second is next 4 bits shifted 4.
    unsigned get_value(m68k_arg_t& arg) const override
    {
        // calclulate value to pass to `fmt_reg_pair`
        auto gen_reg_value = [](auto const p) -> decltype(p->value())
            {
                if (!p)
                {
                    std::cout << "val_pair::gen_reg_value: nullptr" << std::endl;
                    return 0;
                }
                auto n = p->value();
                if (p->kind() == RC_ADDR)
                    n += 8;
                return n;
            };
        auto reg1 = gen_reg_value(arg.reg_p);
        auto reg2 = gen_reg_value(arg.outer.get_p<m68k_reg_t>());

        return reg1 | (reg2 << 4);
    }

    void set_arg(m68k_arg_t& arg, unsigned value) const override
    {
        auto find_reg = [](auto value) -> m68k_reg_t const&
            {
                auto rc = value & 8 ? RC_ADDR : RC_DATA;
                uint16_t reg_num = value & 7;
                return m68k_reg_t::find(rc, reg_num);
            };

        // calculate expression value from machine code
        arg.reg_p  = &find_reg(value);
        arg.outer =   find_reg(value >> 4);
        arg.set_mode(MODE_PAIR);
    }
    
    const bool addr_ok{};
};

// REGSET Functions: Allow single register or regset of appropriate kind()
struct val_regset : m68k_mcode_t::val_t
{
    using regset_t = typename base_t::arg_t::regset_t;
    static constexpr bool have_regset = !std::is_void_v<regset_t>;

    constexpr val_regset(uint8_t kind = {}, uint8_t reverse = {})
                : kind{kind}, reverse(reverse) {}
    
    fits_result ok(m68k_arg_t& arg, expr_fits const& fits) const override
    {
        switch (arg.mode()) {
            case MODE_IMMEDIATE:
            case MODE_IMMED_QUICK:
                // XXX test for fixed in range
                return fits.yes;
            case MODE_REGSET:
                if (auto rs_p = arg.regset_p)
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
                if (arg.reg_p->kind() == kind)
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
                if constexpr (have_regset)
                {
                    // create temporary register-set from single register
                    arg.set_mode(MODE_IMMED_QUICK);
                    regset_t rs(*arg.reg_p);
                    return rs.value(reverse); 
                }
            case MODE_REGSET:
                return arg.regset_p->value(reverse);
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
    uint8_t reverse;
};

struct val_bitfield : m68k_mcode_t::val_t
{
    // `is_register` is MSB of both offset & width fields
    static constexpr auto BF_FIELD_SIZE = 6;
    static constexpr auto BF_REG_BIT    = (1 << (BF_FIELD_SIZE - 1));
    static constexpr auto BF_MASK       = BF_REG_BIT - 1;

    fits_result ok(m68k_arg_t& arg, expr_fits const& fits) const override
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
                    return m68k_reg_t::find(RC_DATA, value & 7);
                return value & BF_MASK;
            };

        arg.outer = get_expr(value);
        value >>= BF_FIELD_SIZE;
        if (value & BF_REG_BIT)
            arg.reg_p = &m68k_reg_t::find(RC_DATA, value & 7);
        else
            arg.expr = value & BF_MASK;
        arg.set_mode(MODE_BITFIELD);
    }
};

// mmu validators
struct val_mmu_fc : m68k_mcode_t::val_t
{
    // MMU `pflush` allows IMMED, data register, SFC, or DFC
    // Allow data register, SFC, or DFC
    fits_result ok(m68k_arg_t& arg, expr_fits const& fits) const override
    {
        if (arg.mode() == MODE_DATA_REG)
            return fits.yes;
        if (arg.mode() != MODE_REG)
            return fits.no;
        if (arg.reg_p->kind() != RC_CTRL)
            return fits.no;

        // M68K defn: SFC = 0, DFC = 1
        if (arg.reg_p->value() > 1)
            return fits.no;
        return fits.yes;
    }
    
    unsigned get_value(m68k_arg_t& arg) const override
    {
        if (arg.mode() == MODE_DATA_REG)
            return arg.reg_num + 8;
        return arg.reg_p->value();
    }

    void set_arg(m68k_arg_t& arg, unsigned value) const override
    {
        // also handle immediate value
        if (value & 0x10)
        {
            arg.expr = value & 0xf;
            arg.set_mode(MODE_IMMEDIATE);
        }

        else if (value & 8)
        {
            arg.reg_p = &m68k_reg_t::find(RC_DATA, value & 7);
            arg.set_mode(MODE_DATA_REG);
        }

        else
        {
            arg.reg_p = &m68k_reg_t::find(RC_CTRL, value & 1);
            arg.set_mode(MODE_REG);
        }
    }
};

struct val_mmu_caches : m68k_mcode_t::val_t
{
    // Support MMU 040 CINV instruction
    fits_result ok(m68k_arg_t& arg, expr_fits const& fits) const override
    {
        if (arg.mode() == MODE_DATA_REG)
            return fits.yes;
        if (arg.mode() != MODE_REG)
            return fits.no;
        if (arg.reg_p->kind() != RC_CTRL)
            return fits.no;

        // M68K defn: SFC = 0, DFC = 1
        if (arg.reg_p->value() > 1)
            return fits.no;
        return fits.yes;
    }
    
    unsigned get_value(m68k_arg_t& arg) const override
    {
        if (arg.mode() == MODE_DATA_REG)
            return arg.reg_num + 8;
        return arg.reg_p->value();
    }

    void set_arg(m68k_arg_t& arg, unsigned value) const override
    {
        // also handle immediate value
        if (value & 0x10)
        {
            arg.expr = value & 0xf;
            arg.set_mode(MODE_IMMEDIATE);
        }

        else if (value & 8)
        {
            arg.reg_p = &m68k_reg_t::find(RC_DATA, value & 7);
            arg.set_mode(MODE_DATA_REG);
        }

        else
        {
            arg.reg_p = &m68k_reg_t::find(RC_CTRL, value & 1);
            arg.set_mode(MODE_REG);
        }
    }
};

// validate MMU register
struct val_mmu_reg : m68k_mcode_t::val_t
{
    constexpr val_mmu_reg(uint8_t r_class) : r_class{r_class} {}
    
    // This is the "unique" validator where sz is needed to validate match.
    // Instead of burdening all other validators with a sz arg, if `sz`
    // is a mis-match, return very large `size()` & let proc-list find correct match
    fits_result ok(m68k_arg_t& arg, expr_fits const& fits) const override
    {
        if (arg.mode() != MODE_REG)
            return fits.no;
        if (arg.reg_p->kind() != r_class)
            return fits.no;
        return fits.yes;
    }
    
    // MMU registers require various size args. number is encoded in reg_value
    // Test for size mismatch
    fits_result size(m68k_arg_t& arg, uint8_t sz
                   , expr_fits const& fits, op_size_t& op_size) const override
    {
        // info about arg encoded in value
        auto value     = arg.reg_p->value();
        auto reg_bytes = value >> 8;

        // check for match
        switch (sz)
        {
            case OP_SIZE_BYTE: if (reg_bytes == 1) return fits.yes; break;
            case OP_SIZE_WORD: if (reg_bytes == 2) return fits.yes; break;
            case OP_SIZE_LONG: if (reg_bytes == 4) return fits.yes; break;
            case OP_SIZE_QUAD: if (reg_bytes == 8) return fits.yes; break;
            default:
                break;
        }
        return fits.no;
    }
    
    unsigned get_value(m68k_arg_t& arg) const override
    {
        // calclulate value to pass to special formatter...
        return arg.reg_p->value();
    }

    void set_arg(m68k_arg_t& arg, unsigned value) const override
    {
        // create register from value passed by special formatter
        arg.reg_p = &m68k_reg_t::find(r_class, value);
        arg.set_mode(MODE_REG);
    }
    
    uint8_t   r_class;
};

// coldfile validators

// `val_subreg` should be paired with `fmt_subreg`. 
// validator return 7 bits: subreg + mode + reg
// NB: 4 lsbs happen to be general register #

static constexpr auto VAL_SUBWORD_UPPER = 1 << 6;
struct val_subreg : m68k_mcode_t::val_t
{
    fits_result ok(m68k_arg_t& arg, expr_fits const& fits) const override
    {
        switch (arg.mode())
        {
            case MODE_SUBWORD_LOWER:
            case MODE_SUBWORD_UPPER:
                return fits.yes;

            default:
                return fits.no;
        }
    }
    
    unsigned get_value(m68k_arg_t& arg) const override
    {
        // calclulate value to insert in machine code
        auto value = arg.reg_p->gen_reg_value();
        if (arg.mode() == MODE_SUBWORD_UPPER)
            value |= VAL_SUBWORD_UPPER;
        return value;
    }

    void set_arg(m68k_arg_t& arg, unsigned value) const override
    {
        auto mode = (value & VAL_SUBWORD_UPPER) ? MODE_SUBWORD_UPPER: MODE_SUBWORD_LOWER;
        auto kind = (value & 8) ? RC_ADDR : RC_DATA;
        arg.reg_p = &m68k_reg_t::find(kind, value & 7);
        arg.set_mode(mode);
    }
};

// test for MAC accumulator
struct val_acc : m68k_mcode_t::val_t
{
    fits_result ok(m68k_arg_t& arg, expr_fits const& fits) const override
    {
        if (arg.mode() != MODE_REG)
            return fits.no;
        
        auto& reg = *arg.reg_p;
        if (reg.kind() != RC_MAC)
            return fits.no;

        if (reg.value() & 4)
            return fits.no;
        
        return fits.yes;
    }
    
    unsigned get_value(m68k_arg_t& arg) const override
    {
        // calclulate value to insert in machine code
        return arg.reg_p->value();
    }

    void set_arg(m68k_arg_t& arg, unsigned value) const override
    {
        // calculate expression value from machine code
        arg.reg_p = &m68k_reg_t::find(RC_MAC, value);
        arg.set_mode(MODE_REG);
    }
    
};

// Allow different MODES that M68K for the CF `BTST` insn only.
// Check that CF may disallow anyway, via 3-word limit
struct val_cf_bit_tst : m68k_mcode_t::val_t
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

// Allow different MODES than M68K. 
// Check that CF may disallow anyway, via 3-word limit
struct val_cf_bit_static : m68k_mcode_t::val_t
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

// use base AM_INDIRECT, but require `has_subword_mask`
struct val_cf_indir_mask : val_am
{
    using base_t = val_am;
    constexpr val_cf_indir_mask() : base_t(AM_INDIRECT) {}

    fits_result ok(m68k_arg_t& arg, expr_fits const& fits) const override
    {
        if (!arg.has_subword_mask)
            return fits.no;
        return base_t::ok(arg, fits);
    }
    
    void set_arg(m68k_arg_t& arg, unsigned value) const override
    {
        arg.has_subword_mask = true;
        return base_t::set_arg(arg, value);
    }
};

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
VAL_GEN (Q_IMMED,    val_range_t<int8_t > , 0); // 8 bits signed (moveq)
VAL_GEN (Q_IMMED16,  val_range_t<int16_t> , 0); // 16 bits signed
VAL_GEN (BIT_IMMED,  val_range_t<uint16_t>, 0); // 16 bits unsigned

VAL_GEN (ADDR_INDIR, val_indirect); 
VAL_GEN (ADDR_DISP,  val_indirect, 1); 

// `INDIRECT` from `m68k_validate_gen.h`, but requires `has_subword_mask` true
VAL_GEN (INDIR_MASK, val_cf_indir_mask);

VAL_GEN (PAIR,       val_pair, 0);      // data pair
VAL_GEN (GEN_PAIR,   val_pair, 1);      // gen-reg pair

// For branches only: sizes args are size of insn, not arg
VAL_GEN (BRANCH    ,  val_branch, 2);       // branch byte/word/long
VAL_GEN (BRANCH_DEL,  val_branch, 0);       // branch byte/word/long DELETE-ABLE
VAL_GEN (BRANCH_WL_DEL,  val_branch_del, 2, 4);    // branch      word/long DELETE-ABLE
VAL_GEN (BRANCH_W  ,  val_branch, 2, 4);    // branch word only 

VAL_GEN (CONTROL_NODIR, val_control_nodir); // allow control. disallow direct

VAL_GEN (MOVEP     ,  val_movep);           // indir or disp.

VAL_GEN (DIR_LONG  , val_dir_long);         // special for move16

VAL_GEN (BITFIELD,   val_bitfield);         // BITFIELD test

VAL_GEN (REGSET       , val_regset, RC_DATA);      // REGSET test: D0 is LSB
VAL_GEN (REGSET_REV   , val_regset, RC_DATA, 16);  // REGSET test: A7 is LSB
VAL_GEN (FP_REGSET    , val_regset, RC_FLOAT);     // REGSET test: FP0 is LSB
VAL_GEN (FP_REGSET_REV, val_regset, RC_FLOAT, 8);  // REGSET test: FP7 is LSB    
VAL_GEN (FC_REGSET,  val_regset, RC_FCTRL);

// MMU validators
VAL_GEN (MMU_FC,    val_mmu_fc);
VAL_GEN (MMU_CACHES,val_mmu_caches);
VAL_GEN (MMU_REG,   val_mmu_reg, RC_MMU_68851);

// coldfire BIT insn validators
VAL_GEN (CF_BIT_TST   , val_cf_bit_tst);     // MEM w/o IMMED (would limit3 knock this out?)
VAL_GEN (CF_BIT_STATIC, val_cf_bit_static);  // CTRL_ALTER w/o abs & index

// coldfire MAC validators
VAL_GEN (REG_UL,     val_subreg);            // test SUBWORD field
VAL_GEN (ACC,        val_acc);               // test if MAC/eMAC accumulator


#undef VAL_GEN
}
#endif

