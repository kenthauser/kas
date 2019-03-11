#ifndef KAS_M68K_M68K_REG_TYPES_H
#define KAS_M68K_M68K_REG_TYPES_H


//
// m68k register patterns:
//
// Each "register value" (as used in m68k.c) consists of a
// three-item tuple < reg_class(ex RX_DATA), reg_num(eg 0), tst(eg: hw::index_full)
// 1) reg_class    ranges from 0..9. `reg_arg_validate` has a hardwired limit of 12.
// 2) The reg_num  ranges from 0..7 for eg data register; has 12-bit value for move.c
// 3) tst          16-bit hw_tst constexpr value
//
// Each register also has a name. Based on command line options, a leading `%` may
// be permitted or requied
//
// A few registers have aliases. Currently the only aliases are fp->a6 & sp->a7
//
// A few registers can have multiple definitions:
// Example: USP can be a `RC_CPU` register when determining if (eg.) move.l a0,usp is allowed
//          USP can also be used as a `RC_CTRL` register in (eg.)   move.c a0, usp
// The two USPs have different `tst` conditions.
//
// Register `PC` is also overloaded.

// Observations:
// - Only a single alias is permitted.
// - No more hand two register definitions can have the same name
// - The "second" register definition index cannot be zero (because defition
//   zero is processed first, it will always be a "first" definition). Thus
//   a index of zero can be used to tell second definition is not present.
// - KAS_STRING register name definitions should always prepend '%'. A simple +1
//   can be used to remove it.
// currently, for M68K there are ~100-120 register definitions

// RC_PC & RC_ZPC both have a single member. Can be merged into RC_CPU

// Also note: `m68k_reg`    is an expression type
//            `m68k_regset` is an expression type
//            `m68k_reg`    needs to export parser to `expr`

#include "expr/expr_types.h"

#include "m68k_hw_defns.h"
#include "target/tgt_reg_type.h"
#include "target/tgt_regset_type.h"


namespace kas::m68k 
{

////////////////////////////////////////////////////////////////////////////
//
// Declare M68K register constants
//
////////////////////////////////////////////////////////////////////////////


// declare the classes of registers (data, address, fp, etc.)
// NB: RC_DATA & RC_ADDR must stay 0 & 1 (they nominate "general registers")
enum
{
      RC_DATA = 0
    , RC_ADDR = 1
    , RC_ZADDR
    , RC_PC         // register class has single member
    , RC_ZPC        // register class has single member
    , RC_CPU
    , RC_CTRL
    , RC_FLOAT
    , RC_FCTRL
    , NUM_REG_CLASS
};

// name special registers for easy access
// NB: REG_CPU_* values are completely arbitrary
enum
{
      REG_CPU_USP
    , REG_CPU_SR
    , REG_CPU_CCR
           
    // coldfire MAC /eMAC
    , REG_CPU_MACSR
    , REG_CPU_MASK
    , REG_CPU_SF_LEFT   // scale factor: "<<"
    , REG_CPU_SF_RIGHT  // scale factor: ">>"

    // coldfire MAC
    , REG_CPU_ACC
    
    // coldfire eMAC
    , REG_CPU_ACC0
    , REG_CPU_ACC1
    , REG_CPU_ACC2
    , REG_CPU_ACC3
    , REG_CPU_ACC_EXT01
    , REG_CPU_ACC_EXT23
};

// name fp control registers for easy access
// values match `fmove.l`
enum
{
      REG_FPCTRL_IAR = 1    // FP Instruction Address Register
    , REG_FPCTRL_SR  = 2    // FP Status Register
    , REG_FPCTRL_CR  = 4    // FP Control Register
};

////////////////////////////////////////////////////////////////////////////
//
// definition of m68k register
//
////////////////////////////////////////////////////////////////////////////

// some assemblers require format such as `%d0`, some require `d0` some allow both
// XXX the prefix is `%` & should be delcared here
enum m68k_reg_prefix { PFX_NONE, PFX_ALLOW, PFX_REQUIRE };

struct m68k_reg_set;
struct m68k_reg_t : tgt::tgt_reg<m68k_reg_t>
{
    using hw_tst         = hw::hw_tst;
    using reg_defn_idx_t = uint8_t;
    using reg_set_t      = m68k_reg_set;

    using base_t::base_t;
    
    // MIT syntax only for now
    static const char *format_name(const char *n, unsigned i = 0)
    {
        if (i == 0)
            return n + 1;
        return {};
    }

};


////////////////////////////////////////////////////////////////////////////
//
// definition of m68k register set
//
////////////////////////////////////////////////////////////////////////////

struct m68k_reg_set : tgt::tgt_reg_set<m68k_reg_set, m68k_reg_t>
{
    using base_t::base_t;

    uint16_t reg_kind(m68k_reg_t const& r) const
    {
        auto kind = r.kind();
        switch (kind)
        {
            case RC_ADDR:
                kind = RC_DATA;
                // FALLSTHRU
            case RC_DATA:
            case RC_FLOAT:
            case RC_FCTRL:
                return kind;
            default:
                return -1;
        }
    }
    
    // convert "register" to bit number in range [0-> (mask_bits - 1)]
    uint8_t reg_bitnum(m68k_reg_t const& r) const
    {
        switch (r.kind())
        {
            case RC_DATA:  return  0 + r.value();
            case RC_ADDR:  return  8 + r.value();
            case RC_FLOAT: return  0 + r.value();
            case RC_FCTRL:
                // floating point control registers are "special"
                switch (r.value())
                {
                    case REG_FPCTRL_CR:  return 12;
                    case REG_FPCTRL_SR:  return 11;
                    case REG_FPCTRL_IAR: return 10;
                }
                // FALLSTHRU
            default:       return {};
        }
    }

    
    std::pair<bool, uint8_t> rs_mask_bits(bool reverse) const
    {
        // For M68K CPU: sixteen bit mask with
        // Normal bit-order: D0 -> LSB, A7 -> MSB
        //
        // For FPU: 8 bit mask with
        // Normal bit-order: FP7 -> LSB, FP0 -> MSB
        //
        // Easiest solution: toggle reverse for FP

        const int mask_bits  = (kind() == RC_FLOAT) ? 8 : 16;
        const bool bit_order = (kind() == RC_FLOAT) ? RS_DIR_MSB0 : RS_DIR_LSB0;

        return { bit_order ^ reverse, mask_bits };
    }
};

using m68k_rs_ref = typename m68k_reg_set::ref_loc_t;
}


// declare X3 parser for `reg_t`
namespace kas::m68k::parser
{
    namespace x3 = boost::spirit::x3;

    // declare parser for M68K token
    using m68k_reg_x3 = x3::rule<struct X_reg, m68k_reg_t>;
    BOOST_SPIRIT_DECLARE(m68k_reg_x3)
}


#endif
