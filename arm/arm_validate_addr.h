#ifndef KAS_ARM_VALIDATE_ADDR_H
#define KAS_ARM_VALIDATE_ADDR_H

/******************************************************************************
 *
 * Instruction argument validation.
 *
 * See `target/tgt_validate.h` for information on virtual base class
 *
 *****************************************************************************/

#include "arm_validate.h"
#include "target/tgt_validate_generic.h"
#include "target/tgt_validate_branch.h"


namespace kas::arm::opc
{
using expr_fits   = expression::expr_fits;
using fits_result = expression::fits_result;
using op_size_t   = core::opcode::op_size_t;
using arg_mode_t  = typename arm_mcode_t::arg_mode_t;

#if 0
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
#endif
struct val_direct: arm_mcode_t::val_t
{
    constexpr val_direct() {}

    fits_result ok(arg_t& arg, expr_fits const& fits) const override
    {
        if (arg.mode() == arg_t::arg_mode_t::MODE_DIRECT)
            return fits.yes;
        return fits.no;
    }
#if 0    
    uint32_t get_value(arg_t& arg) const override
    {
        return {};
    }

    void set_arg(arg_t& arg, uint32_t value) const override
    {
        arg.expr = {};
    }
#endif
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

//
// ARM5 Adddressing mode 2: Load and Store word or unsigned byte
//
// Also validates addressing mode 3: LDR/STR suffixes H, SH, SB, D
// Also validates addressing mode 4: LDC/STC/LDC2/STC2

// Indirect arguments come in three varieties (and the ARM names):
//  1) offset    : REG + reg, shifted, or const offset
//  2) pre-index : REG + reg, shifted, const offset with write back
//  3) post-index: "REG INDIRECT" + reg, shifted or const offset
//
//  Constant offset formats are supported by ELF relocations. Others must resolve
//  with constants. Offset format identified via `arg.indir.is_offset()` true
//
// NB: out-of-range values are checked during KBFD reloc generation

struct val_indir : arm_mcode_t::val_t
{
    // require constexpr default ctor
    constexpr val_indir() {} 

    fits_result ok(arg_t& arg, expr_fits const& fits) const override
    {
        switch (arg.mode())
        {
            // allow indirect addressing modes
            case arg_mode_t::MODE_REG_INDIR:
                return fits.yes;
            
            // try convert direct arg to pc-relative indirect
            case arg_mode_t::MODE_DIRECT:
                return fits.yes;
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
        // xlate DIRECT to INDIRECT with: reg = R15, p_flag set, offset -8
        if (arg.mode() == arg_mode_t::MODE_DIRECT)
            return 0x10f0000;       // offset (with -8 base) passed via reloc
        
        // first calculate upper word
        // base 4-bits are "indirect base register" (ie Rn)
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

        // XXX needs work
        // set shifter value if set
        // NB: `arm7_value()` is zero if shift not set
        value += arg.shift.arm7_value();
        if (arg.indir.r_flag)
            value += arg.indir.reg;         // LSBs
        
        // NB: offset is handled by `reloc`
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
// These addressing modes are used by {LDR,STR}{,B}T
struct val_post_index : val_indir
{
    fits_result ok(arg_t& arg, expr_fits const& fits) const override
    {
#if 0
        // XXX is test complete??
        if (arg.mode() != arg_mode_t::MODE_REG_INDIR)
            return fits.no;
#endif
        // XXX should do base first to allow direct???
        // p-flag indicates offset or pre-indexed addressing
        if (arg.indir.p_flag)
            return fits.no;

        return val_indir::ok(arg, fits);    // apply base limits
    }
};

// allow subset of address-mode-2 addressing modes
// These addressing modes are used by PLD
struct val_indir_offset : val_indir
{
    fits_result ok(arg_t& arg, expr_fits const& fits) const override
    {
        switch (arg.mode())
        {
            // allow offset addressing modes
            case arg_mode_t::MODE_REG_INDIR:
                if (!arg.indir.is_offset())
                    break;
                return fits.yes;
            
            // try convert direct arg to pc-relative indirect
            case arg_mode_t::MODE_DIRECT:
                return fits.yes;
            default:
                break;
        }
        return fits.no;
    }
};

// ARM5 Adddressing mode 3: subset of ARM5 addressing mode 2
// addressing mode 3: no shifts, and offset is limited to 8-bits fixed
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

struct val_regl_update : val_reg
{
    using base_t = val_reg;
    constexpr val_regl_update() : base_t(RC_GEN
                                       , val_reg::all_regs
                                       , arg_t::arg_mode_t::MODE_REG_UPDATE
                                       )
                                 {}

    fits_result ok(arg_t& arg, expr_fits const& fits) const override
    {
        // check for proper mode & proper reg class using base_t
        if (base_t::ok(arg, fits) == fits.yes)
            if (arg.reg_p->value(RC_GEN) < 8)
                return fits.yes;
        return fits.no;
    }
};

struct val_regset_l : arm_mcode_t::val_t
{
    constexpr val_regset_l (uint8_t reg = {}) : reg_num(reg)
                                              , reg_bit(1 << reg)
                                              {}

    fits_result ok(arg_t& arg, expr_fits const& fits) const override
    {
        if (arg.mode() != arg_mode_t::MODE_REGSET)
            return fits.no;

        // see if only the LSBs are set
        auto regset_h = arg.regset_p->value() &~ 0xff;

        if (!regset_h)
            return fits.yes;

        // a MSB is set, see if it's OK
        if (regset_h &~ reg_bit)
            return fits.no;

        return fits.yes;
    }
    
    unsigned get_value(arg_t& arg) const override
    {
        auto n = arg.regset_p->value();
        if (n > 0xff)       // is a high bit set?
            n = (n & 0xff) | 0x100;     // record additional high register
        return n ;
    }

    void set_arg(arg_t& arg, unsigned value) const override
    {
        // calculate expression value from machine code
        if (value & 0x80)
            value = (value & 0xff) | reg_bit;
        arg.expr = value;
        arg.set_mode(arg_mode_t::MODE_IMMED_QUICK);
    }
    
    uint16_t reg_num;
    uint16_t reg_bit;
};

struct val_thumb_indir_base : val_indir
{
    using base_t = val_indir;
    
    // require constexpr default ctor
    constexpr val_thumb_indir_base () {} 

    // allow pre-index and offset modes (without write-back) only  
    fits_result ok(arg_t& arg, expr_fits const& fits) const override
    {
        auto result = base_t::ok(arg, fits);    // see if an ARM indir
        if (result != fits.yes)
            return fits.no;
        if (arg.shift)              // no shift
            return fits.no;
        if (!arg.indir.p_flag)      // require offset or pre-index
            return fits.no;
        if (!arg.indir.u_flag)      // require plus (up)
            return fits.no;
        if (arg.indir.w_flag)       // no write-back
            return fits.no;
        return fits.yes;
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

struct val_indir_5 : val_thumb_indir_base
{
    using base_t = val_thumb_indir_base;
    
    constexpr val_indir_5(uint8_t shift = {}, uint8_t reloc = false)
            : shift(shift)
            , reloc (reloc)
            , mask((1 << shift) - 1)
            , max((1 << (5 + shift)) - 1)
            {};

    fits_result ok(arg_t& arg, expr_fits const& fits) const override
    {
        if (base_t::ok(arg, fits) != fits.yes)
            return fits.no;
        
        // base register must be low general register
        if (arg.reg_p->value(RC_GEN) & 8)
            return fits.no;

        // indirect reg must not be present
        if (arg.indir.r_flag)
            return fits.no;
        
        // offset must be 5-bit positive value, shifted 
        if (auto p = arg.get_fixed_p())
        {
            auto n = *p;
            if (n & mask)
                return fits.no; // must be multiple of obj size 
            return fits.fits(n, 0, max);
        }

        // last test: if relocs allowed, permit
        if (reloc)
            return fits.yes;    // ok if relocs allowed
            
        return fits.no;         // expression must be constant
    }
    
    // return 8-bit value for ARM5 A6.5.1 Format 1 [Rn, 5-bit]
    uint32_t get_value(arg_t& arg) const override
    {
        auto n = arg.reg_p->value(RC_GEN);
        if (auto p = arg.get_fixed_p())
            // shifted 6 bits in opcode - 3 bits for register - size
            n += *p << (6 - 3 - shift);
        return n;
    }
    void set_arg(arg_t& arg, uint32_t value) const override
    {
        arg.reg_p = &arm_reg_t::find(RC_GEN, value & 7);
        arg.expr  = (value &~ 7) >>  (3 - shift);
        arg.indir.p_flag = true;
        arg.indir.u_flag = true;
    }

    uint8_t shift;
    uint8_t mask;
    uint8_t max;
    uint8_t reloc;
};


struct val_indir_l : val_thumb_indir_base
{
    using base_t = val_thumb_indir_base;

    fits_result ok(arg_t& arg, expr_fits const& fits) const override
    {
        if (base_t::ok(arg, fits) != fits.yes)
            return fits.no;
        
        // base register must be low general register
        if (arg.reg_p->value(RC_GEN) & 8)
            return fits.no;

        // indirect reg must be present, require low register
        if (!arg.indir.r_flag || (arg.indir.reg & 8))
            return fits.no;

        // offset must be zero
        if (auto p = arg.get_fixed_p())
        {
            if (*p == 0) return fits.yes;
        }
            
        return fits.no;     // expression must be constant
    }

    // return 8-bit value for ARM5 A6.5.1 Format 2 [Rn, Rm]
    uint32_t get_value(arg_t& arg) const override
    {
        auto n  = arg.reg_p->value(RC_GEN);
             n += arg.indir.reg << 3;
        return n << 3;
    }

    void set_arg(arg_t& arg, uint32_t value) const override
    {
        arg.set_mode(arg_t::arg_mode_t::MODE_REG_INDIR);
        // set base register
        arg.reg_p = &arm_reg_t::find(RC_GEN, value & 7);
        // set indirect register, mode = reg_offset (plus)
        arg.indir.reg    = (value >> 6) & 7;
        arg.indir.r_flag = true;
        arg.indir.p_flag = true;
        arg.indir.u_flag = true;
    }
};

struct val_offset_8 : val_thumb_indir_base
{
    using base_t = val_thumb_indir_base;
    
    // declare 10-bit limits: stored in opcode as signed 8-bits (prescaled)
    static constexpr auto offset8_w_max = +(1 << 10) - 1;
    static constexpr auto offset8_w_min = 0;

    constexpr val_offset_8(uint8_t reg = -1) : reg(reg) {};

    fits_result ok(arg_t& arg, expr_fits const& fits) const override
    {
        if (base_t::ok(arg, fits) != fits.yes)
            return fits.no;

        if (arg.indir.r_flag)
            return fits.no;

        if (arg.reg_p->value(RC_GEN) != reg)
            return fits.no;

        // XXX PC fiddling to get word aligned...
        // XXX may need to use reloc to emit...

        return fits.fits(arg.expr, offset8_w_min, offset8_w_max); 
    }

    uint32_t get_value(arg_t& arg) const override
    {
        // XXX processor "aligns" address to word boundry before calculation
        // XXX analyze.
        if (auto p = arg.get_fixed_p())
            return *p >> 2;

        return 0;
    }
    void set_arg(arg_t& arg, uint32_t value) const override
    {
        arg.reg_p = &arm_reg_t::find(RC_GEN, reg);
        arg.expr  = value * 4;
        //arg.set_mode(arg_mode_t::MODE_DIRECT);
    }

    uint8_t reg;
};

// The `thb_branch*` validators work with the `arm_opc_branch` opcode
template <unsigned BITS>
struct val_tmb_branch : arm_mcode_t::val_t
{
    // out-of-range displacement handled by linker
    fits_result ok(arg_t& arg, expr_fits const& fits) const override
    {
        // NB: there is `reloc` available for unresolved branches
        // NB: `ok` only driven before addresses resolved -- just say YES
        switch (arg.mode())
        {
            case arg_mode_t::MODE_DIRECT:
                return fits.yes;

            default:
                break;
        };
        return fits.no;
    }
    
    fits_result size(arg_t& arg
                   , uint8_t sz
                   , expr_fits const& fits
                   , op_size_t& op_size) const override
    {
        return fits.yes;
    }

};

//using val_indir_pc_8 = val_offset_8;
//using val_indir_sp_8 = val_offset_8;
}
#endif
