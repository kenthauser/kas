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
#include "target/tgt_validate_generic.h"
#include "target/tgt_validate_branch.h"


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

struct val_branch : tgt::opc::tgt_val_branch<val_branch, z80_mcode_t>
{
    using base_t::base_t;

    // max branch instruction size: z80 is 1 byte opcode, 1 byte displacement 
    constexpr uint8_t max(z80_mcode_t const&) const 
    {
        return 2;
    }
};

// validate 8-bit general registers, plus (HL), (IX+n), (IY+n)
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
            // register args are coded as mode() == MODE_REG
            if (arg.reg_p->kind(RC_GEN) == RC_GEN)
                return fits.yes;
            break;

        // allow (HL)
        case MODE_REG_INDIR:
            if (arg.reg_p->kind(RC_IDX) == RC_IDX)
                return fits.yes;
            break;

        case MODE_REG_INDIR_IX:
        case MODE_REG_INDIR_IY:
        case MODE_REG_OFFSET_IX:
        case MODE_REG_OFFSET_IY:
            return fits.yes;
        }
        return fits.no;
    }
    
    // registers by themselves have no size. But IX/IY offsets do
    fits_result size(z80_arg_t& arg, z80_mcode_t const & mc, stmt_info_t const& info, expr_fits const& fits, op_size_t& insn_size) const override
    {
        switch (arg.mode())
        {
            default:
                break;
            case MODE_REG_INDIR_IX:
            case MODE_REG_INDIR_IY:
            case MODE_REG_OFFSET_IX:
            case MODE_REG_OFFSET_IY:
                insn_size += 1;         // single byte offset
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
                return arg.reg_p->value(RC_GEN);
        }
    }

    bool has_data(z80_arg_t& arg) const override
    {
        switch (arg.mode())
        {
            case MODE_REG_INDIR_IX:
            case MODE_REG_INDIR_IY:
            case MODE_REG_OFFSET_IX:
            case MODE_REG_OFFSET_IY:
                return true;
            default:
                return false;
        }
    }

    void set_arg(z80_arg_t& arg, unsigned value) const override
    {
        // set reg to general register, HL, IX, or IY as appropriate
        if (value != 6)
            arg.reg_p = &z80_reg_t::find(RC_GEN, value);
        else if (arg.prefix)
            arg.reg_p = &z80_reg_t::find(RC_IDX, arg.prefix);
        else
            arg.reg_p = &z80_reg_t::find(RC_DBL, 2);
        
        //std::cout << "val_reg_gen::set: value = " << +value;
        //std::cout << " -> reg = " << arg.reg << std::endl;
    }
};

// 16-bit accumulators are HL, IX, IY
// 16-bit double registers BC, DE, HL have values 0, 1, 2
// double register value of `3` overloaded as either AF or SP.
// Specify "class" (RC_AF, RC_SP) enable DBL registers and specify overload
struct val_reg_dbl: z80_mcode_t::val_t
{
    constexpr val_reg_dbl(int16_t r_class = -1) : r_class(r_class) {}

    fits_result ok(z80_arg_t& arg, expr_fits const& fits) const override
    {
        switch (arg.mode())
        {
        default:
            break;

        case MODE_REG:
            if (r_class > 0 && arg.reg_p->kind(r_class) == r_class)
                return fits.yes;
            if (arg.reg_p->kind(RC_DBL) != RC_DBL)
                break;
            
            // test if HL
            if (arg.reg_p->value(RC_DBL) == 2)          
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
            if (arg.reg_p->value(RC_IDX) == arg.prefix)
                return fits.yes;
            break;
        }
        return fits.no;
    }
    
    unsigned get_value(z80_arg_t& arg) const override
    {
        //std::cout << "val_reg_dbl::get_value: mode = " << +arg.mode();
        //std::cout << ", r_class = " << +r_class << std::endl;
        if (arg.reg_p->kind(RC_IDX) == RC_IDX)
            return 2;

        // mirror testes in `ok`
        if (arg.reg_p->kind(r_class) == r_class)
            return arg.reg_p->value(r_class);
        return arg.reg_p->value(RC_DBL);
    }
    
    void set_arg(z80_arg_t& arg, unsigned value) const override
    {
        //std::cout << "val_reg_dbl::set_arg: mode = " << +arg.mode();
        //std::cout << ", r_class = " << +r_class;
        //std::cout << ", value = " << value << std::endl;

        // NB: weird RC_AF & RC_SP have only values `3`. otherwise use RC_DBL
        if (arg.mode() != MODE_REG)
            arg.reg_p = &z80_reg_t::find(RC_IDX, arg.prefix);
        else if (value == 3)
            arg.reg_p = &z80_reg_t::find(r_class, 3);
        else
            arg.reg_p = &z80_reg_t::find(RC_DBL, (uint16_t)value);
    }

    int16_t r_class;
};

// Indirect jmps: Allow (HL), (IX), (IY)
struct val_indir_idx : z80_mcode_t::val_t
{
    constexpr val_indir_idx() {}

    fits_result ok(z80_arg_t& arg, expr_fits const& fits) const override
    {
        switch (arg.mode())
        {
        default:
            break;

        case MODE_REG_INDIR:
            if (arg.reg_p->kind(RC_DBL) != RC_DBL)
                break;
            
            // test if HL
            if (arg.reg_p->value(RC_DBL) == 2)          
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
            if (arg.reg_p->value(RC_IDX) == arg.prefix)
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
            arg.reg_p = &z80_reg_t::find(RC_DBL, 2);
        else
            arg.reg_p = &z80_reg_t::find(RC_IDX, arg.prefix);
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
        if (arg.reg_p->kind(RC_DBL) != RC_DBL)
            return fits.no;
        if (arg.reg_p->value(RC_DBL) > 1)
            return fits.no;
        return fits.yes;
    }
    
    unsigned get_value(z80_arg_t& arg) const override
    {
        // value is zero or 1
        return arg.reg_p->value(RC_DBL);
    }
    
    void set_arg(z80_arg_t& arg, unsigned value) const override
    {
        // value is zero or 1
        arg.reg_p = &z80_reg_t::find(RC_DBL, value);
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
        if (arg.reg_p->kind(RC_CC) != RC_CC)
            return fits.no;
        if (arg.reg_p->value() >= 4)
            return fits.no;
        return fits.yes;
    }
    
    unsigned get_value(z80_arg_t& arg) const override
    {
        // get CC value
        return arg.reg_p->value(RC_CC);
    }
    
    void set_arg(z80_arg_t& arg, unsigned value) const override
    {
        arg.reg_p = &z80_reg_t::find(RC_CC, value);
    }
};

struct val_direct: z80_mcode_t::val_t
{
    constexpr val_direct() {}

    fits_result ok(z80_arg_t& arg, expr_fits const& fits) const override
    {
        if (arg.mode() == MODE_DIRECT)
            return fits.yes;
        return fits.no;
    }

    fits_result size(z80_arg_t& arg, z80_mcode_t const& mc, stmt_info_t const& info
                   , expr_fits const& fits, op_size_t& op_size) const override
    {
        op_size += 2;
        return fits.yes;
    }
};

// extend `tgt_val_range` to allow MODE_DIRECT for bit operations
struct val_range : tgt::opc::tgt_val_range<z80_mcode_t> 
{
    using base_t = tgt_val_range<z80_mcode_t>;
    using base_t::base_t;

    fits_result ok(arg_t& arg, expr_fits const& fits) const override
    {
        if (arg.mode() == arg_mode_t::MODE_DIRECT)
            return base_t::range_ok(arg, fits);
        return base_t::ok(arg, fits);
    }
};

template <typename T>
using val_range_t = tgt::opc::tgt_val_range_t<z80_mcode_t, T, val_range>;


// validate RST instruction: require multiple of 8, range 0..0x38
struct val_restart : z80_mcode_t::val_t
{
    constexpr val_restart() {}

    fits_result ok(z80_arg_t& arg, expr_fits const& fits) const override
    {
        // range is only for direct args
        if (arg.mode() != MODE_DIRECT)
            return fits.no;
        
        auto p = arg.get_fixed_p();

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
        arg.set_mode(MODE_IMMED_QUICK);
        if (auto p = arg.get_fixed_p())
            return *p >> 3;
        return 0;
    }
    
    void set_arg(z80_arg_t& arg, unsigned value) const override
    {
        arg.set_mode(MODE_IMMED_QUICK);
        arg.expr = value << 3;
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
    fits_result size(z80_arg_t& arg, z80_mcode_t const& mc, stmt_info_t const& info
                   , expr_fits const& fits, op_size_t& insn_size) const override
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
VAL_REG(REG         , RC_GEN); // Named Registers
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

VAL_REG(REG_IDX     , RC_IDX);     // allows HL, IX, IY

// validations which require method
VAL_GEN(REG_GEN     , val_reg_gen);     // allows (HL), (IX+n), (IY+n) and 8-bit general registers
VAL_GEN(REG_DBL_SP  , val_reg_dbl, RC_SP);  // IDX + SP
VAL_GEN(REG_DBL_AF  , val_reg_dbl, RC_AF);  // IDX + AF

VAL_GEN(INDIR       , val_indir, 2);    // 16-bit indirect
VAL_GEN(INDIR_8     , val_indir, 1);    // 8-bit indirect (I/O instructions)

VAL_GEN(INDIR_BC_DE , val_indir_bc_de);
VAL_GEN(INDIR_IDX   , val_indir_idx);

// validate JR condition codes
VAL_GEN(JR_CC       , val_jrcc);

// For branches only: config_sizes areare size of insn, not arg
VAL_GEN (BRANCH    ,  val_branch, 2, 2);    // branch byte in range (DJNZ)
VAL_GEN (BRANCH_DEL,  val_branch, 0);       // branch byte/word/long DELETE-ABLE

// Validate numeric arguments
VAL_GEN(DIRECT      , val_direct);

VAL_GEN(IMMED_8     , val_range_t<std::int8_t >);
VAL_GEN(IMMED_16    , val_range_t<std::int16_t>);

VAL_GEN(IMMED_IM    , val_range, 0, 2);
VAL_GEN(BIT_NUM     , val_range, 0, 7);
VAL_GEN(IMMED_RST   , val_restart);

}

#undef VAL_REG
#undef VAL_GEN
#endif
