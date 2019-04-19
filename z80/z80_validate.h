#ifndef KAS_Z80_VALIDATE_H
#define KAS_Z80_VALIDATE_H

/******************************************************************************
 *
 * Instruction argument validation.
 *
 * See `target/tgt_validate.h` for information on virtual base class
 *
 *****************************************************************************/

#include "z80_mcode.h"
#include "target/tgt_validate.h"


namespace kas::z80::opc
{
using expr_fits   = expression::expr_fits;
using fits_result = expression::fits_result;
using op_size_t   = core::opcode::op_size_t;

// Derive common validators from generic templates

struct val_reg : tgt::opc::tgt_val_reg<val_reg, z80_mcode_t>
{
    using base_t::base_t;
};

struct val_range : tgt::opc::tgt_val_range<val_range, z80_mcode_t>
{
    using base_t::base_t;
};


// validate (HL), (IX+n), (IY+n), and optionally 8-bit general registers
struct val_reg_gen: z80_mcode_t::val_t
{
    constexpr val_reg_gen() {}

    // test argument against validation
    fits_result ok(z80_arg_t& arg, expr_fits const& fits) const override
    {
        switch (arg.mode())
        {
        default:
            break;

        case MODE_REG:
            // register args are coded as.mode() = MODE_REG
            if (arg.reg.kind(RC_GEN) == RC_GEN)
                return fits.yes;
            break;

        case MODE_REG_INDIR_IX:
        case MODE_REG_INDIR_IY:
        case MODE_REG_OFFSET_IX:
        case MODE_REG_OFFSET_IY:
            // only 1 type of prefix register allowed per instruction
            //std::cout << "val_reg_gen: reg = " << arg.reg << " pfx = " << +arg.prefix << std::endl;
            if (arg.reg.value() != arg.prefix)
                break;
            return fits.yes;
                
        case MODE_REG_INDIR:
            // allow (HL)
            //std::cout << "val_reg_gen: reg (HL) = " << arg.reg << " pfx = " << +arg.prefix << std::endl;
            if (arg.reg.kind(RC_DBL) != RC_DBL)
                break;
            if (arg.prefix)
                break;                      // can't mix (HL) & IX/IY
            if (arg.reg.value() == 2)       // HL
                return fits.yes;
            break;
        }
        return fits.no;
    }
    
    // registers by themselves have no size. But IX/IY offsets do
    fits_result size(z80_arg_t& arg, uint8_t sz, expr_fits const& fits, op_size_t& insn_size) const override
    {
        switch (arg.mode())
        {
            default:
                break;
            case MODE_REG_INDIR_IX:
            case MODE_REG_INDIR_IY:
            case MODE_REG_OFFSET_IX:
            case MODE_REG_OFFSET_IY:
                insn_size += 1;
                break;
        }
        return fits.yes;
    }

    unsigned get_value(z80_arg_t& arg) const override
    {
        switch (arg.mode())
        {
            case MODE_REG_INDIR:
            case MODE_REG_INDIR_IX:
            case MODE_REG_INDIR_IY:
            case MODE_REG_OFFSET_IX:
            case MODE_REG_OFFSET_IY:
                return 6;                   // register code for (HL)
            default:
                return arg.reg.value(RC_GEN);
        }
    }
    
    void set_arg(z80_arg_t& arg, unsigned value) const override
    {
        // set reg to general register, HL, IX, or IY as appropriate
        if (value != 6)
            arg.reg = z80_reg_t(RC_GEN, value);
        else if (arg.prefix)
            arg.reg = z80_reg_t(RC_IDX, arg.prefix);
        else
            arg.reg = z80_reg_t(RC_DBL, 2);
        
        //std::cout << "val_reg_gen::set: value = " << +value;
        //std::cout << " -> reg = " << arg.reg << std::endl;
    }
};

// 16-bit accumulators are HL, IX, IY
// 16-bit double registers BC, DE, HL have values 0, 1, 2
// double register value of `3` overloaded as either AF or SP.
// Specify "class" (RC_AF, RC_SP) enable DBL registers and specify overload
struct val_reg_idx: z80_mcode_t::val_t
{
    constexpr val_reg_idx(int16_t r_class = -1) : r_class(r_class) {}

    fits_result ok(z80_arg_t& arg, expr_fits const& fits) const override
    {
        switch (arg.mode())
        {
        default:
            break;

        case MODE_REG:
            if (r_class > 0 && arg.reg.kind(r_class) == r_class)
                return fits.yes;
            if (arg.reg.kind(RC_DBL) != RC_DBL)
                break;
            
            // test if HL
            if (arg.reg.value(RC_DBL) == 2)          
            {
                // can't mix HL & IX/IY
                if (arg.prefix)
                    break;
            }
            
            // here for BC/DE
            else
            {
                if (r_class < 0)
                    break;
            }
            return fits.yes;
            break;

        case MODE_REG_IX:
        case MODE_REG_IY:
            // only one prefix reg allowed per instruction
            if (arg.reg.value(RC_IDX) == arg.prefix)
                return fits.yes;
            break;
        }
        return fits.no;
    }
    
    unsigned get_value(z80_arg_t& arg) const override
    {

        if (arg.reg.kind(RC_IDX) == RC_IDX)
            return 2;
        if (arg.reg.kind(RC_DBL) == RC_DBL)
            return arg.reg.value(RC_DBL);
        return 3;
    }
    
    void set_arg(z80_arg_t& arg, unsigned value) const override
    {
        if (arg.mode() != MODE_REG)
            arg.reg = z80_reg_t(RC_IDX, arg.prefix);
        else if (value != 3)
            arg.reg = z80_reg_t{RC_DBL, (uint16_t)value};
        else if (r_class == RC_AF)
            arg.reg = z80_reg_t{RC_AF, 3};
        else
            arg.reg = z80_reg_t(RC_SP, 3);
    }

    int16_t r_class;
};

// Indirect jmps: Allow (HL), (IX), (IY)
struct val_indir_idx: z80_mcode_t::val_t
{
    constexpr val_indir_idx() {}

    fits_result ok(z80_arg_t& arg, expr_fits const& fits) const override
    {
        switch (arg.mode())
        {
        default:
            break;

        case MODE_REG_INDIR:
            if (arg.reg.kind(RC_DBL) != RC_DBL)
                break;
            
            // test if HL
            if (arg.reg.value(RC_DBL) == 2)          
            {
                // can't mix HL & IX/IY
                if (arg.prefix)
                    break;
            }
            
            return fits.yes;
            break;

        case MODE_REG_INDIR_IX:
        case MODE_REG_INDIR_IY:
            // only one prefix reg allowed per instruction
            if (arg.reg.value(RC_IDX) == arg.prefix)
                return fits.yes;
            break;
        }
        return fits.no;
    }
    
    unsigned get_value(z80_arg_t& arg) const override
    {
        return 2;   // HL
    }
    
    void set_arg(z80_arg_t& arg, unsigned value) const override
    {
        if (arg.mode() == MODE_REG_INDIR)
            arg.reg = z80_reg_t(RC_DBL, 2);
        else
            arg.reg = z80_reg_t(RC_IDX, arg.prefix);
    }
};
    
struct val_indir_bc_de : val_reg
{
    constexpr val_indir_bc_de() : val_reg(RC_DBL) {}

    // test argument against validation
    fits_result ok(z80_arg_t& arg, expr_fits const& fits) const override
    {
        if (arg.mode() != MODE_REG_INDIR)
            return fits.no;
        if (arg.reg.kind(RC_DBL) != RC_DBL)
            return fits.no;
        if (arg.reg.value(RC_DBL) > 1)
            return fits.no;
        return fits.yes;
    }
    
    unsigned get_value(z80_arg_t& arg) const override
    {
        // value is zero or 1
        return arg.reg.value(RC_DBL);
    }
    
    void set_arg(z80_arg_t& arg, unsigned value) const override
    {
        // value is zero or 1
        arg.reg = z80_reg_t(RC_DBL, value);
    }
};

    
struct val_jrcc : val_reg
{
    constexpr val_jrcc() : val_reg(RC_CC) {}

    // test argument against validation
    fits_result ok(z80_arg_t& arg, expr_fits const& fits) const override
    {
        if (arg.mode() != MODE_REG)
            return fits.no;
        if (arg.reg.kind(RC_CC) != RC_CC)
            return fits.no;
        if (arg.reg.value() >= 4)
            return fits.no;
        return fits.yes;
    }
};
// validate RST instruction: require multiple of 8, rante 0..0x38
struct val_restart : z80_mcode_t::val_t
{
    constexpr val_restart() {}

    fits_result ok(z80_arg_t& arg, expr_fits const& fits) const override
    {
        // range is only for direct args
        if (arg.mode() != MODE_DIRECT)
            return fits.no;
        
        auto p = arg.expr.get_fixed_p();

        // require fixed argument
        if (!p)
            return fits.no;
        
        // require multiple of 8, range 0..0x38
        if (~(7 << 3) & *p)
            return fits.no;
    
        return fits.yes;
    }

    unsigned get_value(z80_arg_t& arg) const override
    {
        if (auto p = arg.expr.get_fixed_p())
            return *p >> 3;
        return 0;
    }
    
    void set_arg(z80_arg_t& arg, unsigned value) const override
    {
        arg.expr = value << 3;
    }
    
    bool all_saved(z80_arg_t& arg) const override
    {
        return true;
    }
};

// 16-bit indirect (memory) or 8-bit indirect (I/O)
struct val_indir : z80_mcode_t::val_t
{
    constexpr val_indir(uint16_t size) : _size(size) {}

    fits_result ok(z80_arg_t& arg, expr_fits const& fits) const override
    {
        if (arg.mode() != MODE_INDIRECT)
            return fits.no;

        if (_size >= 2)
            return fits.yes;

        return fits.ufits_sz(arg.expr, _size); 
    }

    // immediates may be inserted in opcode, or added data
    fits_result size(z80_arg_t& arg, uint8_t sz, expr_fits const& fits, op_size_t& insn_size) const override
    {
        insn_size += _size;
        return ok(arg, fits);
    }

    uint16_t _size;
};

// use preprocessor to define string names used in definitions & debugging...
#define VAL_REG(NAME, ...) using NAME = _val_reg<KAS_STRING(#NAME), __VA_ARGS__>
#define VAL_GEN(NAME, ...) using NAME = _val_gen<KAS_STRING(#NAME), __VA_ARGS__>

template <typename NAME, typename T, int...Ts>
using _val_gen = meta::list<NAME, T, meta::int_<Ts>...>;

template <typename NAME, int...Ts>
using _val_reg = _val_gen<NAME, val_reg, Ts...>;


// register-class and register-specific validations
VAL_REG(REG         , RC_GEN);
// Named Registers
VAL_REG(REG_A       , RC_GEN, 7);
VAL_REG(REG_C       , RC_GEN, 1);
VAL_REG(REG_I       , RC_I);
VAL_REG(REG_R       , RC_R);

VAL_REG(REG_AF      , RC_AF);
VAL_REG(REG_BC      , RC_DBL, 0);
VAL_REG(REG_DE      , RC_DBL, 1);
VAL_REG(REG_HL      , RC_DBL, 2);
VAL_REG(REG_SP      , RC_SP);

VAL_REG(CC          , RC_CC);

VAL_REG(INDIR_SP    , RC_SP,  3, MODE_REG_INDIR);
VAL_REG(INDIR_C     , RC_GEN, 1, MODE_REG_INDIR);

// validations which require method
VAL_GEN(REG_GEN     , val_reg_gen);     // allows (HL), (IX+n), (IY+n) and 8-bit general registers
VAL_GEN(REG_IDX     , val_reg_idx);     // allows HL, IX, IY
VAL_GEN(REG_DBL_SP  , val_reg_idx, RC_SP);  // IDX + SP
VAL_GEN(REG_DBL_AF  , val_reg_idx, RC_AF);  // IDX + AF

VAL_GEN(INDIR       , val_indir, 2);    // 16-bit indirect
VAL_GEN(INDIR_8     , val_indir, 1);    // 8-bit indirect (I/O instructions)

VAL_GEN(INDIR_BC_DE , val_indir_bc_de);
VAL_GEN(INDIR_IDX   , val_indir_idx);

VAL_GEN(JR_CC       , val_jrcc);
VAL_GEN(DIRECT      , val_range, 2, 0, 0xffff);

VAL_GEN(IMMED_8     , val_range, 1, -(1<< 7), (1<< 8)-1);
VAL_GEN(IMMED_16    , val_range, 2, -(1<<15), (1<<16)-1);
VAL_GEN(IMMED_IM    , val_range, 0, 0, 2);
VAL_GEN(BIT_NUM     , val_range, 0, 0, 7);
VAL_GEN(IMMED_RST   , val_restart);

}

#undef VAL_REG
#undef VAL_GEN
#endif
