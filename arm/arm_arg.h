#ifndef KAS_ARM_ARM_ARG_DEFN_H
#define KAS_ARM_ARM_ARG_DEFN_H

// Declare arm argument & arg MODES

#include "arm_reg_types.h"
#include "target/tgt_arg.h"

namespace kas::arm
{

// forward declare type
struct arm_arg_t;

// Declare argument "modes"
// These must be "defined" as they are referenced in `target/*` code.
// The `enum` values need not be in any preset order.
// The `NUM_ARG_MODES` is the highest value in use. Required enums may
// have values above `NUM_ARG_MODES` if not used
enum arm_arg_mode : uint8_t
{
// Standard Modes
      MODE_NONE             // 0 when parsed: indicates missing: always zero
    , MODE_ERROR            // 1 set error message
    , MODE_DIRECT           // 2 direct address
    , MODE_INDIRECT         // 3 indirect address (NB: NOT ARM)
    , MODE_IMMEDIATE        // 4 immediate arg (expression)
    , MODE_IMMED_QUICK      // 5 immediate arg (constant stored in opcode)
    , MODE_REG              // 6 register
    , MODE_REG_INDIR        // 7 register indirect (NB: All ARM register indirects)
    , MODE_REGSET           // 8 register set 
    , MODE_BRANCH           // 9 branch instruction

// Processor required modes
    , MODE_REG_UPDATE       // 10 update register after use (usage ex: STM)
    , MODE_SHIFT            // 11 shift instruction

// Immediate Sub-fields
    , MODE_IMMED_LOWER      // :lower:
    , MODE_IMMED_UPPER      // :upper:
    , MODE_IMMED_BYTE_0     // :lower0_7:#
    , MODE_IMMED_BYTE_1     // :lower8_15:#
    , MODE_IMMED_BYTE_2     // :upper0_7:#
    , MODE_IMMED_BYTE_3     // :upper8_15:#

// Required enumeration
    , NUM_ARG_MODES
    , MODE_REG_OFFSET       // register + offset (indirect) (NB: NOT ARM)
    , NUM_BRANCH = 1        // only 1 branch insn
    
// special handles...
    , MODE_IMMED_MODE_FIRST = MODE_IMMED_LOWER
    , MODE_IMMED_MODE_LAST  = MODE_IMMED_BYTE_3
};

// allow `arm_shift` to be initialized & treaded as `uint8_t`
struct arm_shift : detail::alignas_t<arm_shift, uint8_t>
{
    using base_t::base_t;

    // immed shift stored in `ext`; gen reg_num also stored in `ext`
    template <typename OS>
    void print(OS& os) const;

    // true iff arg is shifted
    operator bool() const
    {
        return value() != 0;
    }

    value_t     ext    : 5;     // shift constant (NB: not relocatable)
    value_t     type   : 2;     // shift type
    value_t     is_reg : 1;     // use register shift
 };
 
// allow `arm_indirect` to be initialized & treaded as `uint16_t`
struct arm_indirect: detail::alignas_t<arm_indirect, uint16_t>
{
    using base_t::base_t;
    constexpr arm_indirect() {};
    
    // ARM7 flags (all shifted 20 bits)
    static constexpr auto W_FLAG = 0x02;    // 1 = write-back
    static constexpr auto U_FLAG = 0x08;    // 1 = up (ie add)
    static constexpr auto P_FLAG = 0x10;    // 1 = pre-index
    static constexpr auto R_FLAG = 0x20;    // 1 = use REGISTER form (LDR/STR) (bit 25)
    static constexpr auto S_FLAG = 0x40;    // 1 = KAS: shift (needed for serialize)
#if 0
    // XXX must determine via MODE_INDIR
    // true iff "indirect" argument
    operator bool() const
    {
        return flags & (W_FLAG | P_FLAG);
    }
#endif

    // shift & immed stored in `arg_t`
    template <typename OS>
    void print(OS& os, arm_arg_t const&) const;
    
    uint8_t     flags{};
    uint8_t     reg{};              // save RC_GEN value, not reg_p
};


// `REG_T` & `REGSET_T` parameters allow `MCODE_T` to lookup types
struct arm_arg_t : tgt::tgt_arg_t<arm_arg_t
                                 , arm_arg_mode
                                 , arm_reg_t
                                 , arm_reg_set_t>
{
    // inherit basic ctors
    using base_t::base_t;

    // declare size of immed args
    static constexpr tgt::tgt_immed_info sz_info [] =
        {
              { 0 }         // 0: Immediate arguments not emited
        };

    auto& immed_info(uint8_t sz) const
    {
        return base_t::immed_info(0);
    }

    // handle non-generic modes 
    const char *set_mode(unsigned mode);
   
    // some non-generic modes are immediate
    bool is_immed() const;
    
    template <typename Inserter, typename ARG_INFO>
    bool serialize(Inserter& inserter, uint8_t sz, ARG_INFO *);
    
    template <typename Reader, typename ARG_INFO>
    void extract(Reader& reader, uint8_t sz, ARG_INFO const*);

    // override default print
    template <typename OS> void print(OS&) const;
    
    arm_shift       shift;      // support shifts
    arm_indirect    indir;      // support register indirect
};

}

#endif
