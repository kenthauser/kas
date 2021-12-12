#ifndef KAS_ARM_VALIDATE_H
#define KAS_ARM_VALIDATE_H

/******************************************************************************
 *
 * Instruction argument validation.
 *
 * See `target/tgt_validate.h` for information on virtual base class
 *
 *****************************************************************************/

#include "arm_mcode.h"
#include "target/tgt_validate_generic.h"
#include "target/tgt_validate_branch.h"


namespace kas::arm::opc
{
using expr_fits   = expression::expr_fits;
using fits_result = expression::fits_result;
using op_size_t   = core::opcode::op_size_t;

//
// Derive common validators from generic templates
//

struct val_reg : tgt::opc::tgt_val_reg<val_reg, arm_mcode_t>
{
    using base_t::base_t;
};

struct val_range: tgt::opc::tgt_val_range<arm_mcode_t, int32_t>
{
    using base_t::base_t;
};

template <unsigned SCALE>
struct val_range_scaled: tgt::opc::tgt_val_range<arm_mcode_t, int32_t, SCALE>
{
    using base_t = tgt::opc::tgt_val_range<arm_mcode_t, int32_t, SCALE>;
    using base_t::base_t;
};

struct val_false : tgt::opc::tgt_val_false<arm_mcode_t> {};

// immediate with "IMMED_UPDATE" mode
struct val_range_update : val_range
{
    using val_range::val_range;
    
    fits_result ok(arg_t& arg, expr_fits const& fits) const override
    {
        // specialize "range" for IMMED_UPDATE args
        switch (arg.mode())
        {
            case arg_mode_t::MODE_IMMED_UPDATE:
                return range_ok(arg, fits);
            default:
                return fits.no;
        }
    }
};

//
// ARM Specific Validators
//

struct val_regset : arm_mcode_t::val_t
{
    fits_result ok(arg_t& arg, expr_fits const& fits) const override
    {
        return arg.mode() == arg_mode_t::MODE_REGSET ? fits.yes : fits.no;
    }
    
    unsigned get_value(arg_t& arg) const override
    {
        switch (arg.mode())
        {
            case arg_mode_t::MODE_IMMEDIATE:
                arg.set_mode(arg_mode_t::MODE_IMMED_QUICK);
                // FALLSTHRU
            case arg_mode_t::MODE_IMMED_QUICK:
                if (auto p = arg.expr.get_fixed_p())
                    return *p;
                return 0;
            case arg_mode_t::MODE_REGSET:
                return arg.regset_p->value();
            default:
            // calclulate value to insert in machine code
                return 0;
        }
    }

    void set_arg(arg_t& arg, unsigned value) const override
    {
        // calculate expression value from machine code
        arg.expr = value;
        arg.set_mode(arg_mode_t::MODE_IMMED_QUICK);
    }
    
};

struct val_regset_single : val_regset
{
    using base_t = val_regset;

    fits_result ok(arg_t& arg, expr_fits const& fits) const override
    {
        if (base_t::ok(arg, fits) == fits.yes)
            if (arg.regset_p->is_single())
                return fits.yes;
        return fits.no;
    }
};

struct val_regset_user : val_regset
{
    fits_result ok(arg_t& arg, expr_fits const& fits) const override
    {
        return arg.mode() == arg_mode_t::MODE_REGSET_USER ? fits.yes : fits.no;
    }
};

// ARM5 Adddressing mode 1: Data processing instructions: Shifts
struct val_shift : arm_mcode_t::val_t
{
    constexpr val_shift() {}

    fits_result ok(arg_t& arg, expr_fits const& fits) const override
    {
        return arg.mode() == arg_mode_t::MODE_SHIFT ? fits.yes : fits.no;
    }

    // standard ARM encoding
    unsigned get_value(arg_t& arg) const override
    {
        return arg.shift.arm7_value();
    }

    void set_arg(arg_t& arg, unsigned value) const override
    {
        arg.shift.arm7_set(value);
    }
};

// ARM5 Adddressing mode 2: Load and Store word or unsigned byte
struct val_indir : arm_mcode_t::val_t
{
    // declare 12-bit limits: stored in opcode as sign + 12-bits offset
    static constexpr auto offset12_max = +((1 << 12) - 1);
    static constexpr auto offset12_min = -((1 << 12) - 1);

    // require constexpr default ctor
    constexpr val_indir() {} 

    fits_result ok(arg_t& arg, expr_fits const& fits) const override
    {
        switch (arg.mode())
        {
            // allow indirect addressing modes
            case arg_mode_t::MODE_REG_INDIR:
            case arg_mode_t::MODE_REG_IEXPR:
                return fits.yes;
            // try convert direct arg to pc-relative indirect
            case arg_mode_t::MODE_DIRECT:
                return fits.disp(arg.expr, offset12_min, offset12_max, 8);
            default:
                break;
        }
        return fits.no;
    }

    // standard ARM encoding (32-bit value)
    // 1. 12 LSBs hold shifter or offset values
    // 2. 4 bits << 16 hold Rn (ie indirect base register)
    // 3. next 6 bits (ie shifted 20) hold following:
    //      R_flag, P_flag, U_flag, B_flag, W_flag, L_flag
    //
    // NB: L_flag && B_flag handled via `info` methods

    // addressing modes are interpreted in `arm_parser_support.h`
    // most `indir` values and modes are validated there

    uint32_t get_value(arg_t& arg) const override
    {
        // xlate DIRECT to INDIRECT with: reg = R15, p_flag set
        if (arg.mode() == arg_mode_t::MODE_DIRECT)
            return 0x10f0000;
        
        // first calculate upper word
        uint32_t value  = arg.reg_p->value(RC_GEN);
        if (arg.indir.p_flag)
            value |= 1 << (24-16);
        if (arg.indir.w_flag)
            value |= 1 << (21-16);
        if (arg.indir.u_flag)
            value |= 1 << (23-16);
        if (arg.indir.r_flag)
            value |= 1 << (25-16);
        value <<= 16;

        // now lower word
#if 0
        // XXX needs work
        value += arg.shift.arm7_value();
        if (arg.indir.r_flag)
            value += arg.indir.reg;
#else
        // offset value
        if (auto p = arg.get_fixed_p())
            value += *p;
#endif
        return value;
    }

    void set_arg(arg_t& arg, uint32_t value) const override
    {
        // analyze upper word values first
        auto msw = value >> 16;
        arg.reg_p = &arm_reg_t::find(RC_GEN, msw & 0xf);

        if (msw & (1 << (24-16)))
        {
            arg.indir.p_flag = true;

            // NB: only set W-flag if P-flag set
            if (msw & (1 << (21-16)))
                arg.indir.w_flag = true;
        }
        if (msw & (1 << (23-16)))
            arg.indir.u_flag = true;
        if (msw & (1 << (25-16)))
            arg.indir.r_flag = true;

#if 0
        // extract lower word values 
        arg.shift.arm7_set(value);
        if (arg.indir.r_flag)
            arg.indir.reg = value & 15;
#else
        arg.expr = value & 0xfff;
#endif
    }
};

// allow subset of address-mode-2 addressing modes
struct val_post_index : val_indir
{
    fits_result ok(arg_t& arg, expr_fits const& fits) const override
    {
        // p-flag indicates offset or pre-indexed addressing
        if (arg.indir.p_flag)
            return fits.no;

        return val_indir::ok(arg, fits);
    }
};

// allow subset of address-mode-2 addressing modes
struct val_indir_offset : val_indir
{
    fits_result ok(arg_t& arg, expr_fits const& fits) const override
    {
        // p_flag == 1 && w_flag == 0 indicates offset addressing
        if (!(arg.indir.p_flag && !arg.indir.w_flag))
            return fits.no;

        return val_indir::ok(arg, fits);
    }
};

// ARM5 Adddressing mode 3: subset of ARM5 addressing mode 2
// addressing mode 3: no shifts, and offset is limited to 8-bits fixed
// NB: addressing mode 2 allows offsets of 12-bits + relocations
struct val_ls_misc : val_indir
{
    fits_result ok(arg_t& arg, expr_fits const& fits) const override
    {
        // shift arg not allowed for misc loads/stores
        if (arg.shift.value() != 0)
            return fits.no;
        // enforce signed 8-bit values
        if (auto p = arg.get_fixed_p())
            if (fits.fits<int8_t>(*p) != fits.yes)
                return fits.no;
        // use common routine
        return val_indir::ok(arg, fits);
    }
   
    // addressing mode 3 is similar to addressing mode 2 w/o shifts
    uint32_t get_value(arg_t& arg) const override
    {
        // first calculate "base" value
        uint32_t value  = val_indir::get_value(arg);

        // override lower 12-bits (shift values)
        value &=~ (1<<12) - 1;
        if (auto p = arg.get_fixed_p())
        {
            value |= *p & 0xf;
            value |= (*p & 0xf0) << 4;
        }
        return value;
    }

    void set_arg(arg_t& arg, uint32_t value) const override
    {
        // mask out "shifter" values, but allow Rm
        val_indir::set_arg(arg, value & ~0xff0);

        // if r_flag clear, LSBs are offset
        if (!arg.indir.r_flag)
            arg.expr = (value & 0xf) | ((value >> 4) & 0xf0);
        else
            arg.expr = {};
    }
};

// ARM5 Adddressing mode 4: subset of ARM5 addressing mode 3
// addressing mode 4: offsets are prescaled by *4
struct val_cp_indir : val_ls_misc
{
    using base_t = val_ls_misc;

    fits_result ok(arg_t& arg, expr_fits const& fits) const override
    {
        // shift arg not allowed for misc loads/stores
        if (arg.shift.value() != 0)
            return fits.no;
        // enforce signed 8-bit values
        if (auto p = arg.get_fixed_p())
            if (fits.fits<int8_t>(*p) != fits.yes)
                return fits.no;
        // use common routine
        return base_t::ok(arg, fits);
    }

    // addressing mode 3 is similar to addressing mode 2 w/o shifts
    uint32_t get_value(arg_t& arg) const override
    {
        // first calculate "base" value
        uint32_t value  = base_t::get_value(arg);

        // override lower 12-bits (shift values)
        value &=~ (1<<12) - 1;
        if (auto p = arg.get_fixed_p())
        {
            value |= *p & 0xf;
            value |= (*p & 0xf0) << 4;
        }
        return value;
    }

    void set_arg(arg_t& arg, uint32_t value) const override
    {
        // mask out "shifter" values, but allow Rm
        base_t::set_arg(arg, value & ~0xff0);

        // if r_flag clear, LSBs are offset
        if (!arg.indir.r_flag)
            arg.expr = (value & 0xf) | ((value >> 4) & 0xf0);
        else
            arg.expr = {};
    }
};

// XXX use standard validator, but special inserter...
// ARM5: addressing mode 3: 8-bit value split into bytes
struct val_imm8: tgt::opc::tgt_val_range<arm_mcode_t, int32_t>
{
    // XXX is there a relocation: may need to validate constant
    constexpr val_imm8(...) : base_t(-255, 255) {}
    
    unsigned get_value(arg_t& arg) const override
    {
        auto value = base_t::get_value(arg);

        // XXX U flag?
        if (auto p = arg.expr.get_fixed_p())
        {
            value += (*p & 0x0f) << 0;
            value += (*p & 0xf0) << 4;
        }
        return value;
    }

    void set_arg(arg_t& arg, unsigned value) const override
    {
        base_t::set_arg(arg, value);
        auto n = ((value & 0xf00) >> 4) + (value & 0x0f);
        arg.expr = n;       // XXX U flag?
    }
};

struct val_arm_branch24 : arm_mcode_t::val_t
{
    // out-of-range displacement handled by linker
    fits_result ok(arg_t& arg, expr_fits const& fits) const override
    {
        return fits.yes;
    }
    
    // set `arg_mode` to MODE_BRANCH
    fits_result size(arg_t& arg, mcode_t const&, stmt_info_t const& info
                   , expr_fits const& fits, op_size_t& op_size) const override
    {
        arg.set_mode(arg_t::arg_mode_t::MODE_BRANCH);
        return fits.yes;
    }

    // XXX these methods need work
    unsigned get_value(arg_t& arg) const override
    {
        auto value = base_t::get_value(arg) - 8;
        return (value >> 2) & 0xfff;
    }

    void set_arg(arg_t& arg, unsigned value) const override
    {
        // convert 24-bit signed to 32-bit signed
        int32_t disp = value << 8;      // move "sign" to MSB.
        base_t::set_arg(arg, (disp >> 6) - 8);
    }
};

// special validator to support `bl` ARM_CALL reloc
struct val_arm_call24 : val_arm_branch24
{
    // indicate `ARM_CALL`: set `arg_mode` to MODE_CALL
    fits_result size(arg_t& arg, mcode_t const&, stmt_info_t const& info
                   , expr_fits const& fits, op_size_t& op_size) const override
    {
        std::cout << "val_arm_call24: size()" << std::endl;
        arg.set_mode(arg_t::arg_mode_t::MODE_CALL);
        return fits.yes;
    }
};

//
// Thumb validators: base on ARM validators
//

struct val_regl : val_reg
{
    using base_t = val_reg;
    constexpr val_regl () : base_t(RC_GEN) {}

    fits_result ok(arg_t& arg, expr_fits const& fits) const override
    {
        // check for proper mode & proper reg class using base_t
        if (base_t::ok(arg, fits) == fits.yes)
            if (arg.reg_p->value(r_class) < 8)
                return fits.yes;

        return fits.no;
    }
};


// use preprocessor to define string names used in definitions & debugging...
#define VAL_REG(NAME, ...) using NAME = _val_reg<KAS_STRING(#NAME), __VA_ARGS__>
#define VAL_GEN(NAME, ...) using NAME = _val_gen<KAS_STRING(#NAME), __VA_ARGS__>

template <typename NAME, typename T, int...Ts>
using _val_gen = meta::list<NAME, T, meta::int_<Ts>...>;

template <typename NAME, int...Ts>
using _val_reg = _val_gen<NAME, val_reg, Ts...>;

// register-class and reg+mode validations
VAL_REG(REG         , RC_GEN);
VAL_REG(REG_UPDATE  , RC_GEN, val_reg::all_regs, parser::MODE_REG_UPDATE);
VAL_REG(FLT_SGL     , RC_FLT_SGL);
VAL_REG(FLT_DBL     , RC_FLT_DBL);

VAL_REG(COPROC      , RC_COPROC);
VAL_REG(CREG        , RC_C_REG);

// Named Registers
VAL_REG(SP          , RC_GEN, 13);
VAL_REG(LR          , RC_GEN, 14);
VAL_REG(PC          , RC_GEN, 15);
VAL_REG(APSR        , RC_CPU, REG_CPU_APSR);
VAL_REG(CPSR        , RC_CPU, REG_CPU_CPSR);
VAL_REG(SPSR        , RC_CPU, REG_CPU_SPSR);

// ARM5 addressing mode validators
// ARM V5: addressing mode 1 validators
VAL_GEN(SHIFT       , val_shift);

// ARM V5: addressing mode 2 validators
VAL_GEN(REG_INDIR   , val_indir);
VAL_GEN(POST_INDEX  , val_post_index);
VAL_GEN(INDIR_OFFSET, val_indir_offset);
VAL_GEN(CP_REG_INDIR, val_cp_indir);

// ARM V5: addressing mode 3 validators
VAL_GEN(LS_MISC     , val_ls_misc);
VAL_GEN(OFFSET8     , val_imm8);

// ARM V5: addressing mode 4 validators
VAL_GEN(REGSET      , val_regset);
VAL_GEN(REGSET_SGL  , val_regset_single);
VAL_GEN(REGSET_USER , val_regset_user);

// Thumb register validators
VAL_GEN(REGL        , val_regl);
VAL_GEN(REGH        , val_regl);    // XXX

VAL_GEN(REGL_INDIR5 , val_indir);
VAL_GEN(REGL_INDIRL , val_indir);
VAL_GEN(PC_INDIR8   , val_indir);
VAL_GEN(SP_INDIR8   , val_indir);
VAL_GEN(REGSET_T    , val_regset);
VAL_GEN(REGSET_TR   , val_regset);

// validate branch and thumb branch displacements
// name by reloc generated. linker validates displacements
VAL_GEN(ARM_JUMP24  , val_arm_branch24);
VAL_GEN(ARM_CALL24  , val_arm_call24);

VAL_GEN(THB_JUMP8   , val_false);
VAL_GEN(THB_JUMP11  , val_false);

// XXX
// unsigned validators
VAL_GEN(U16         , val_range, 0, (1<<16) - 1);
VAL_GEN(U12         , val_range, 0, (1<<12) - 1);
VAL_GEN(U4          , val_range, 0, (1<<4)  - 1);


VAL_GEN(IMM24       , val_range, 0, (1<<12) - 1);
VAL_GEN(IMM16       , val_range, 0, (1<<12) - 1);
VAL_GEN(IMM5        , val_range, 0, (1<<5 ) - 1);
VAL_GEN(IMM4        , val_range, 0, (1<<4 ) - 1);
//VAL_GEN(IMM5_UPDATE , val_imm_update, 5);
VAL_GEN(IMM5_UPDATE , val_false);

// Thumb immediates
VAL_GEN(IMMED_3        , val_range, 0, (1 << 3) - 1);
VAL_GEN(IMMED_5        , val_range, 0, (1 << 5) - 1);
VAL_GEN(IMMED_7        , val_range, 0, (1 << 7) - 1);
VAL_GEN(IMMED_8        , val_range, 0, (1 << 8) - 1);

VAL_GEN(IMMED_7_4        , val_range_scaled<2>, 0, (1 << 7) - 1);
VAL_GEN(IMMED_8_4        , val_range_scaled<2>, 0, (1 << 8) - 1);

// 8-bit value shifted by multiple of 4. special relocations
VAL_GEN(IMM8_4      , val_false);

VAL_GEN(ZERO        , val_range, 0, 0);
VAL_GEN(LABEL       , val_range, 0, (1<<12) - 1);
VAL_GEN(SHIFT_Z     , val_range, 0, 0);
VAL_GEN(SHIFT_NZ    , val_range, 0, 0);
VAL_GEN(REG_OFFSET  , val_range, 0, 0);
VAL_GEN(SP_UPDATE   , val_range, 0, 0);

VAL_GEN(FLAGS_AIF    , val_range, 0, 0);
VAL_GEN(FLAGS_ENDIAN , val_range, 0, 0);
VAL_GEN(ONES        , val_range, 0, 0);
VAL_GEN(DMB_OPTION  , val_range, 0, 0);
VAL_GEN(ISB_OPTION  , val_range, 0, 0);

VAL_GEN(LSL         , val_range, 0, 0);
VAL_GEN(ASR         , val_range, 0, 0);
VAL_GEN(BIT         , val_range, 0, 0);
VAL_GEN(BIT4        , val_range, 0, 0);
VAL_GEN(CSPR_FLAGS  , val_range, 0, 0);
VAL_GEN(SSPR_FLAGS  , val_range, 0, 0);
VAL_GEN(XTEND_ROR   , val_range, 0, 0);
}

#undef VAL_REG
#undef VAL_GEN
#endif
