#ifndef KAS_M68K_VALIDATE_REG_H
#define KAS_M68K_VALIDATE_REG_H

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
    // insert & extract values from opcode
    virtual unsigned get_value(arg_t& arg)           const { return {}; }
    virtual void     set_arg  (arg_t& arg, unsigned) const {}
 *
 *
 *
 *****************************************************************************/

#include "m68k_mcode.h"
#include "target/tgt_validate_generic.h"
#include <typeinfo>

namespace kas::m68k::opc
{
using namespace meta;

// validate function signature: pointer to function returning `op_size_t`
using expr_fits   = expression::expr_fits;
using fits_result = expression::fits_result;
using op_size_t   = core::opcode::op_size_t;

// declare addressing mode constants
enum {
      AM_GEN        = 1 <<  0
    , AM_DATA       = 1 <<  1
    , AM_MEMORY     = 1 <<  2
    , AM_CTRL       = 1 <<  3 
    , AM_ALTERABLE  = 1 <<  4
    , AM_IMMED      = 1 <<  5
    , AM_DIRECT     = 1 <<  6
    , AM_REGSET     = 1 <<  7
    , AM_PDEC       = 1 <<  8
    , AM_PINC       = 1 <<  9
// coldfire MAC validator bits
    , AM_INDIRECT   = 1 << 10       // mode = 2/3/4/5
    , AM_GEN_REG    = 1 << 11       // mode = 0/1
    , AM_MAC_GEN    = 1 << 12       // mode = 0/1/7-4
// default for no match
    , AM_NONE       = 1 << 15
    };


// use preprocessor to define string names used in definitions & debugging...
#define VAL(NAME, ...)      using NAME = _val_am <KAS_STRING(#NAME), __VA_ARGS__>
#define VAL_REG(NAME, ...)  using NAME = _val_reg<KAS_STRING(#NAME), __VA_ARGS__>

// validate based on "access mode"
struct val_am : m68k_mcode_t::val_t
{
    constexpr val_am(uint16_t am, int8_t _mode = -1, int8_t _reg = -1) 
                : match {am}, _mode(_mode), _reg(_reg) {}

    // test argument against validation
    fits_result ok(m68k_arg_t& arg, expr_fits const& fits) const override
    {
        // basic AM mode match
        if ((arg.am_bitset() & match) != match)
            return fits.no;
        
        // disallow PC-relative if arg is "ALTERABLE"
        if (arg.mode() == MODE_DIRECT)
            if (match & AM_ALTERABLE)
                arg.set_mode(MODE_DIRECT_ALTER);

        // if expression doesn't fit, will error out later.
        // better matches selected in `m68k_arg_t::size()`
        return fits.yes;
    }

    // bytes required by arg: registers never require extra space, but "extensions" may..
    fits_result size(m68k_arg_t& arg, m68k_mcode_t const& mc, m68k_stmt_info_t const& info
                   , expr_fits const& fits, op_size_t& op_size) const override
    {
        // regset baked into base size
        if (match == AM_REGSET)
            return fits.yes;

        auto arg_size = arg.size(info.sz(mc), fits);
        if (arg_size.is_error())
            return fits.no;

        op_size += arg_size;

        // if `arg_size` not fixed, it's a maybe.
        auto result = fits.yes;
        if (arg_size.min != arg_size.max)
            result = fits.maybe;
        
        // infer "maybe": if `inner` or `outer` not resolved, it's a maybe.
        // don't worry about size being tested: just being tested for "resolved"
        if (result == fits.yes)
            if (fits.fits<int16_t>(arg.expr) == fits.maybe)
                result = fits.maybe;
        if (result == fits.yes)
            if (fits.fits<int16_t>(arg.outer) == fits.maybe)
                result = fits.maybe;

        // use arg generic routine to see if coldfire limit exceeded
        //return coldfire_limit(result, size_p);
        return result;
    }

    // return 6-bit mode+reg value
    unsigned get_value(m68k_arg_t& arg) const override
    {
        // return MODE+REG as 6-bits
        auto mode = (_mode >= 0) ? _mode : arg.cpu_mode(); 
        auto reg  = (_reg  >= 0) ? _reg  : arg.cpu_reg();
        return (mode << 3) + reg;
    }

    // interpret 6-bit mode+reg value
    void set_arg(m68k_arg_t& arg, unsigned value) const override
    {
        // need set mode of some sort...
        auto mode = (_mode >= 0) ? _mode : (value >> 3) & 7;
        auto reg  = (_reg  >= 0) ? _reg  : value & 7;

        if (mode == 7)
            mode += reg;
        else
            arg.reg_num = reg;

        arg.set_mode(mode);
    }

    uint16_t match;
    int8_t   _mode;
    int8_t   _reg;
};

// validate based on "register class" or specific "register"
struct val_reg : m68k_mcode_t::val_t
{
    constexpr val_reg(uint16_t r_class, int16_t r_num = -1) : r_class{r_class}, r_num(r_num) {}

    // test argument against validation
    fits_result ok(m68k_arg_t& arg, expr_fits const& fits) const override
    {
        // must special case ADDR_REG & DATA_REG as these are
        // stored as a "arg.mode()": (magic number alert...).
        if (r_class <= RC_ADDR)
            return (r_class == arg.mode()) ? fits.yes : fits.no;

        // Other register classes are coded as mode = MODE_REG
        else if (arg.mode() != MODE_REG)
            return fits.no;

        // here validating MODE_REG arg matches. validate if REG class matches desired
       auto& reg = *arg.reg_p;
       if (reg.kind() != r_class)
            return fits.no;

        // unparseable registers don't match
        if (reg.is_unparseable())
            return fits.no;

        // here reg-class matches. Test reg-num if specified
        // test if testing for rc_class only (ie. rc_value < 0)
        if (r_num < 0)
            return fits.yes;

        // not default: look up actual rc_value
        if (reg.value(r_class) == r_num)
            return fits.yes;

        return fits.no;
    }
    
    // registers by themselves have no size. Don't override default size() method 
    unsigned get_value(m68k_arg_t& arg) const override
    {
        // return reg-num + mode for general registers
        if (r_class <= RC_ADDR)
            return (r_class << 3) + arg.reg_num;

        return arg.reg_p->value(r_class);
    }

    void set_arg(m68k_arg_t& arg, unsigned value) const override
    {
        // if reg_number specified, override value
        if (r_num >= 0)
            value = r_num;
        
        if (r_class <= RC_ADDR)
        {
            //arg.cpu_mode = r_class;
            arg.reg_num  = value & 7;
            arg.set_mode(r_class);
        }
        else
        {
            arg.reg_p = &m68k_reg_t::find(r_class, value);
            arg.set_mode(MODE_REG);
        }
    }

    uint16_t r_class;
    int16_t  r_num;
};

template <typename N, int...Ts>
using _val_am  = meta::list<N, val_am , meta::int_<Ts>...>;

template <typename N, int...Ts>
using _val_reg = meta::list<N, val_reg, meta::int_<Ts>...>;


VAL(GEN,            AM_GEN);
VAL(ALTERABLE,      AM_ALTERABLE);
VAL(DATA,           AM_DATA);
VAL(DATA_ALTER,     AM_DATA | AM_ALTERABLE);
VAL(CONTROL,        AM_CTRL);
VAL(CONTROL_ALTER,  AM_CTRL | AM_ALTERABLE);
VAL(MEM,            AM_MEMORY);
VAL(MEM_ALTER,      AM_MEMORY | AM_ALTERABLE);
VAL(IMMED,          AM_IMMED, MODE_IMMEDIATE);  // mode == 7-4
VAL(POST_INCR,      AM_PINC,  MODE_POST_INCR);  // mode == 3
VAL(PRE_DECR,       AM_PDEC,  MODE_PRE_DECR);   // mode == 4
VAL(DIRECT,         AM_DIRECT);                 // mode == 7-0 & 7-1
VAL(CONTROL_INDIR,  AM_CTRL | AM_INDIRECT);     // mode == 2/5

// register-class and register-specific validations 
VAL_REG(DATA_REG,   RC_DATA);
VAL_REG(ADDR_REG,   RC_ADDR);
VAL_REG(CTRL_REG,   RC_CTRL);
VAL_REG(FP_REG,     RC_FLOAT);
VAL_REG(FCTRL_REG,  RC_FCTRL);

VAL_REG(FPIAR,      RC_FCTRL, REG_FPCTRL_IAR);
VAL_REG(SR,         RC_CPU,   REG_CPU_SR);
VAL_REG(CCR,        RC_CPU,   REG_CPU_CCR);
VAL_REG(USP,        RC_CPU,   REG_CPU_USP);

// MMU registers
VAL_REG(MMU_VAL,    RC_MMU_68851, 0x150);

// coldfire MAC validators
VAL(GEN_REG,        AM_GEN_REG);            // mode == 0/1
VAL(MAC_GEN,        AM_MAC_GEN);            // mode == 0/1/7-4 (for MAC/eMAC)
VAL(INDIRECT,       AM_INDIRECT);           // mode == 2/3/4/5

VAL_REG(MAC,        RC_MAC);
VAL_REG(CF_SHIFT,   RC_SHIFT);
VAL_REG(MACSR,      RC_MAC,   REG_MAC_MACSR);

#undef VAL
#undef VAL_REG

}


namespace kas::m68k
{

// NB: _am_bitset is `mutable` so this calculation is done once...
uint16_t m68k_arg_t::am_bitset() const
{
    using namespace opc;        // pickup AM_* definitions

    // magic numbers below are from m68k opcode formats...
    static constexpr uint16_t cpu_mode_to_access_mode[] = {
          AM_GEN | AM_DATA | AM_GEN_REG          | AM_ALTERABLE | AM_MAC_GEN    // 0: data_reg
        , AM_GEN           | AM_GEN_REG          | AM_ALTERABLE | AM_MAC_GEN    // 1: addr_reg
        , AM_GEN | AM_DATA | AM_MEMORY | AM_CTRL | AM_ALTERABLE | AM_INDIRECT   // 2: addr_indirect
        , AM_GEN | AM_DATA | AM_MEMORY | AM_PINC | AM_ALTERABLE | AM_INDIRECT   // 3: post_incr
        , AM_GEN | AM_DATA | AM_MEMORY | AM_PDEC | AM_ALTERABLE | AM_INDIRECT   // 4: pre_decr
        , AM_GEN | AM_DATA | AM_MEMORY | AM_CTRL | AM_ALTERABLE | AM_INDIRECT   // 5: addr_displacement
        , AM_GEN | AM_DATA | AM_MEMORY | AM_CTRL | AM_ALTERABLE                 // 6: index
        , AM_GEN | AM_DATA | AM_MEMORY | AM_CTRL | AM_ALTERABLE | AM_DIRECT     // 7-0: abs word
        , AM_GEN | AM_DATA | AM_MEMORY | AM_CTRL | AM_ALTERABLE | AM_DIRECT     // 7-1: abs long
        , AM_GEN | AM_DATA | AM_MEMORY | AM_CTRL                                // 7-2: pc displacement
        , AM_GEN | AM_DATA | AM_MEMORY | AM_CTRL                                // 7-3: pc index
        , AM_GEN | AM_DATA | AM_MEMORY | AM_IMMED | AM_REGSET   | AM_MAC_GEN    // 7-4: immed
        };

    if (!_am_bitset)
    {
        unsigned am_index = mode_normalize();
        switch (am_index)
        {
            default:
                // OK if directly-mapped mode...
                if (am_index <= MODE_IMMEDIATE)
                    break;      // break iff OK.
            // FALLSTHRU           
            case MODE_SUBWORD_LOWER:
            case MODE_SUBWORD_UPPER:
                // ERROR: no match. Pick non-zero value which never matches
                return _am_bitset = AM_NONE;
            
            case MODE_REGSET:
                return _am_bitset = AM_REGSET;
        }
        _am_bitset = cpu_mode_to_access_mode[am_index];

    }
    return _am_bitset;
}
}
#endif
