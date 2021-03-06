#ifndef KAS_M68K_M68K_REG_TYPES_H
#define KAS_M68K_M68K_REG_TYPES_H

////////////////////////////////////////////////////////////////////////////
//
// Declare M68K register constants
//
// Each "register value" (as used in m68k.c) consists of a
// three-item tuple { reg_class(ex RC_DATA), reg_num(eg 0), tst(eg: hw::index_full) }
// 1) reg_class    ranges from 0..15 `reg_arg_validate` (4-bits)
// 2) The reg_num  ranges from 0..7 for eg data register; has 12-bit value for movec
// 3) tst          16-bit hw_tst constexpr value
//
// Each register also has a name. Based on command line options, a leading `%` may
// be permitted or required
//
// A few registers have aliases. Currently the only aliases are fp == a6 & sp == a7
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

////////////////////////////////////////////////////////////////////////////

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

    , RC_MMU_68851  // MMUs are similar, but different...
    , RC_MMU_030
    , RC_MMU_040
    , RC_MAC        // coldfire MAC/eMAC registers
    , RC_SHIFT      // coldfire MAC/eMAC shift values
    , NUM_REG_CLASS
};

// name special registers for easy access
// NB: enum values are completely arbitrary
enum
{
      REG_CPU_USP
    , REG_CPU_SR
    , REG_CPU_CCR
};

// values are used in 3-bit "register" field of mac move insn
enum
{
    // coldfire MAC/eMAC
      REG_MAC_MACSR     = 4
    , REG_MAC_MASK      = 6

    // coldfire MAC
    , REG_MAC_ACC       = 0
    
    // coldfire eMAC
    , REG_MAC_ACC0      = 0
    , REG_MAC_ACC1      = 1
    , REG_MAC_ACC2      = 2
    , REG_MAC_ACC3      = 3
    , REG_MAC_ACC_EXT01 = 5
    , REG_MAC_ACC_EXT23 = 7
};

// name shift "registers"
enum
{
      REG_SHIFT_NONE  = 0   // scale factor: NONE
    , REG_SHIFT_LEFT  = 1   // scale factor: "<<"
    , REG_SHIFT_RSVD  = 2   // scale factor: reserved
    , REG_SHIFT_RIGHT = 3   // scale factor: ">>"
};

// name fp control registers
// NB: values match `fmove.l`
enum
{
      REG_FPCTRL_IAR = 0    // FP Instruction Address Register
    , REG_FPCTRL_SR  = 1    // FP Status Register
    , REG_FPCTRL_CR  = 2    // FP Control Register
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

// reg_set holds `register mask` or `register + offset` values
// for M68K, only holds register mask values. Special case "general" registers
template <typename Ref>
struct m68k_reg_set : tgt::tgt_reg_set<m68k_reg_set<Ref>, m68k_reg_t, Ref>
{
    using base_t = tgt::tgt_reg_set<m68k_reg_set<Ref>, m68k_reg_t, Ref>;
    using base_t::base_t;

    
    // data & address are in same register-set number space
    // NB: Templated to defer instantiation until `m68k_reg_t` defined
    template <typename REG_T>
    uint16_t reg_kind(REG_T const& r) const
    {
        auto kind = r.kind();
        if (kind == RC_ADDR)
            return RC_DATA;
        return kind;
    }

    template <typename REG_T>
    uint8_t  reg_num(REG_T const& r) const
    {
        auto n = r.value();
        if (r.kind() == RC_ADDR)
            n += 8;
        return n;
    }
};

// alias the "reference" used for for register_set type
using m68k_rs_ref    = core::ref_loc_tpl<m68k_reg_set>;
using m68k_reg_set_t = typename m68k_rs_ref::object_t;

// M68K register type definition is regular
struct m68k_reg_t : tgt::tgt_reg<m68k_reg_t, KAS_STRING("M68K")
                               , hw::cpu_defs_t, m68k_reg_set_t>
{
    using reg_defn_idx_t = uint8_t;
    using base_t::base_t;       // use inherited ctors


    // return "general register" value (for m68k_extension & other uses)
    // NB: undefined value if register is not general register
    uint16_t gen_reg_value() const;

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
