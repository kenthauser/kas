#ifndef KAS_M68K_M68K_REG_TYPES_H
#define KAS_M68K_M68K_REG_TYPES_H


//
// m68k register patterns:
//
// Each "register value" (as used in m68k.c) consists of a
// three-item tuple < reg_class(ex RC_DATA), reg_num(eg 0), tst(eg: hw::index_full)
// 1) reg_class    ranges from 0..15 `reg_arg_validate` (4-bits)
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
// - No more than two register definitions can have the same name
// - The "second" register definition index cannot be zero (because definition
//   zero is processed first, it will always be a "first" definition). Thus
//   a index of zero can be used to tell second definition is not present.
// - KAS_STRING register name definitions should always prepend '%'. A simple +1
//   can be used to remove it.
// currently, for M68K there are ~100-120 register definitions

// RC_PC & RC_ZPC both have a single member. Can be merged into RC_CPU ???

// Also note: `m68k_reg`    is an expression variant type
//            `m68k_regset` is an expression variant type
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
enum : std::uint8_t
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

// name fp control registers
// NB: values match `fmove.l`
enum
{
      REG_FPCTRL_IAR = 1    // FP Instruction Address Register
    , REG_FPCTRL_SR  = 2    // FP Status Register
    , REG_FPCTRL_CR  = 4    // FP Control Register
};

////////////////////////////////////////////////:////////////////////////////
//
// definition of m68k register
//
////////////////////////////////////////////////////////////////////////////

// some assemblers require format such as `%d0`, some require `d0` some allow both
enum m68k_reg_prefix { PFX_ALLOW, PFX_REQUIRE, PFX_NONE };

// forward declare CRTP register type
struct m68k_reg_t;

// reg_set holds [register + offset] values
template <typename Ref>
struct m68k_reg_set : tgt::tgt_reg_set<m68k_reg_set<Ref>, m68k_reg_t, Ref>
{
    using base_t = tgt::tgt_reg_set<m68k_reg_set<Ref>, m68k_reg_t, Ref>;
    using base_t::base_t;
};

// alias the "reference" used for for register_set type
using m68k_rs_ref    = core::ref_loc_tpl<m68k_reg_set>;
using m68k_reg_set_t = typename m68k_rs_ref::object_t;


// define `m68k_reg` types as tokens
//using tok_m68k_reg = parser::token_defn_t<KAS_STRING("M68K_REG"), m68k_reg_t>;

// M68K register type definition is regular
struct m68k_reg_t : tgt::tgt_reg<m68k_reg_t, KAS_STRING("M68K"), m68k_reg_set_t>
{
    using hw_tst         = hw::hw_tst;
    using reg_defn_idx_t = uint8_t;
    //using token_t        = tok_m68k_reg;
    
    using base_t::base_t;       // use inherited ctors

    // modify name according to `reg_prefix` as required
    // NB: if `PFX_ALLOW`, both names are added to register name parser
    static const char *format_name(const char *, unsigned i = 0);

    // initialize with out-of-line definition at start of assemble
    static m68k_reg_prefix reg_pfx;
};

// add to `expr` expression types
using m68k_reg_ref = core::ref_loc_t<m68k_reg_t>;

}
#endif
