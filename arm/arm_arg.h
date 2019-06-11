#ifndef KAS_ARM_ARM_ARG_DEFN_H
#define KAS_ARM_ARM_ARG_DEFN_H

// Declare arm argument & arg MODES

#include "arm_reg_types.h"
#include "target/tgt_arg.h"

namespace kas::arm
{

// Declare argument "modes"
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
    , MODE_REG_OFFSET       // 8 register + offset (indirect) (NB: NOT ARM)
    , MODE_REGSET           // 9 register set 

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
    
// special handles...
    , MODE_IMMED_MODE_FIRST = MODE_IMMED_LOWER
    , MODE_IMMED_MODE_LAST  = MODE_IMMED_BYTE_3
};

// forward declare for print methods
struct arm_arg_t;

// allow `arm_shift` to be initialized & treaded as `uint16_t`
struct arm_shift : detail::alignas_t<arm_shift, uint16_t>
{
    using base_t::base_t;

    // immed stored in `ext`; print signature matches `indirect`
    template <typename OS>
    void print(OS& os, arm_arg_t const&) const;

    // true iff arg is shifted
    operator bool() const
    {
        return value() != 0;
    }

    uint16_t    ext    : 8;     // shift constant (NB: not relocatable)
    uint16_t    reg    : 4;     // general register #
    uint16_t    type   : 2;     // shift type
    uint16_t    is_reg : 1;     // use register shift
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

    // true iff "indirect" argument
    operator bool() const
    {
        return flags & (W_FLAG | P_FLAG);
    }

    // shift & immed stored in `arg_t`
    template <typename OS>
    void print(OS& os, arm_arg_t const&) const;
    
    uint8_t    flags { U_FLAG };
    uint8_t    reg   {};
};


// `REG_T` & `REGSET_T` args also allow `MCODE_T` to lookup types
struct arm_arg_t : tgt::tgt_arg_t<arm_arg_t, arm_arg_mode, arm_reg_t, arm_reg_set>
{
    // inherit default & error ctors
    using base_t::base_t;
    using error_msg = typename base_t::error_msg;

    // declare size of immed args
    static constexpr tgt::tgt_immed_info sz_info [] =
        {
              { 0 }         // 0: Immediate arguments not emited after machine code
        };

    // handle non-generic modes 
    const char *set_mode(unsigned mode);
   
    // some non-generaic modes are immediate
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
