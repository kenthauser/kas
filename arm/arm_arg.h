#ifndef KAS_ARM_ARM_ARG_DEFN_H
#define KAS_ARM_ARM_ARG_DEFN_H

// Declare arm argument & arg MODES

#include "arm_reg_types.h"
#include "target/tgt_arg.h"

namespace kas::arm::parser
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
    , MODE_IMMEDIATE        // 3 immediate arg (expression)
    , MODE_IMMED_QUICK      // 4 immediate arg (constant stored in opcode)
    , MODE_REG              // 5 register
    , MODE_REG_INDIR        // 6 register indirect
    , MODE_REGSET           // 7 register set 
    , MODE_BRANCH           // 8 branch instruction

// Processor required modes
    , MODE_REG_UPDATE       //  9 update register after use (usage ex: STM)
    , MODE_SHIFT            // 10 shift instruction
    , MODE_REGSET_USER      // 11 regster set (user regs, not current regs)
    , MODE_IMMED_UPDATE     // 12 immediate with "update" flag
    , MODE_CP_OPTION        // 13 co-processor option (load/store coprocessor)

// Immediate Sub-fields
    , MODE_IMMED_LOWER      // :lower:
    , MODE_IMMED_UPPER      // :upper:
    , MODE_IMMED_BYTE_0     // :lower0_7:#
    , MODE_IMMED_BYTE_1     // :lower8_15:#
    , MODE_IMMED_BYTE_2     // :upper0_7:#
    , MODE_IMMED_BYTE_3     // :upper8_15:#

// Special modes to support particular RELOCs
    , MODE_CALL             // emit `R_ARM_CALL` reloc

// Required enumerations
    , NUM_ARG_MODES
    , MODE_REG_P_OFFSET     // register + positive offset (NB: NOT ARM)
    , MODE_REG_M_OFFSET     // register + negative offset (NB: NOT ARM)
    , MODE_INDIRECT         // indirect address (NB: NOT ARM)
    
// special handles...
    , MODE_IMMED_MODE_FIRST = MODE_IMMED_LOWER
    , MODE_IMMED_MODE_LAST  = MODE_IMMED_BYTE_3
    
    , NUM_BRANCH = 1        // only 1 branch insn
};

// allow `arm_shift` to be initialized & treated as `uint8_t`
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

    // utilities to xlate to/from ARM7 binary formats
    uint16_t arm7_value() const;
    void     arm7_set(uint16_t);

    value_t     ext    : 5;     // shift constant (NB: not relocatable)
    value_t     type   : 2;     // shift type
    value_t     is_reg : 1;     // use register shift
 };
 
// allow `arm_indirect` to be initialized & treated as `uint8_t`
struct arm_indirect: detail::alignas_t<arm_indirect, uint8_t>
{
    using base_t::base_t;
   
    // some of these flags are stored as "complements" to help
    // with parser heirarchy
    value_t reg     : 4;    // save RC_GEN value, not reg_p
    value_t p_flag  : 1;    // pre-index (or offset) addressing
    value_t w_flag  : 1;    // write-back for pre-index
    value_t u_flag  : 1;    // UP-flag: (offset or reg is added)
    value_t r_flag  : 1;    // has register

    // true if the "offset" format of indirect addressing
    bool is_offset() const { return p_flag && !w_flag; }

    // shift & immed stored in `arg_t`
    template <typename OS>
    void print(OS& os, arm_arg_t const&) const;
};


// `REG_T` & `REGSET_T` parameters allow `MCODE_T` to lookup types
struct arm_arg_t : tgt::tgt_arg_t<arm_arg_t
                                 , arm_arg_mode
                                 , arm_reg_t
                                 , arm_reg_set_t
                                 , struct arm_stmt_info_t>
{
    // inherit basic ctors
    using base_t::base_t;

    // declare size of immed args
    static constexpr tgt::tgt_immed_info sz_info [] =
        {
              { 0 }         // 0: Immediate arguments not emited
        };

    // handle non-generic modes 
    const char *set_mode(unsigned mode);
   
    // some non-generic modes are immediate
    bool is_immed() const;

    // arm args are always embedded, never appended
    void emit       (core::core_emit&, uint8_t) const {};
    void emit_immed (core::core_emit&, uint8_t) const {};
    void emit_float (core::core_emit&, tgt::tgt_immed_info const&) const {};
    
    template <typename Inserter, typename ARG_INFO>
    bool serialize(Inserter& inserter, uint8_t sz, ARG_INFO *, bool has_val);
    
    template <typename Reader, typename ARG_INFO>
    void extract(Reader& reader, uint8_t sz, ARG_INFO const*, bool has_val);

    // override default print
    template <typename OS> void print(OS&) const;
    
    arm_shift       shift;      // support shifts
    arm_indirect    indir;      // support register indirect
};

}

#endif
