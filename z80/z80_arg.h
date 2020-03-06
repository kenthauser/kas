#ifndef KAS_Z80_Z80_ARG_DEFN_H
#define KAS_Z80_Z80_ARG_DEFN_H

// Declare z80 argument & arg MODES

#include "z80_reg_types.h"
#include "target/tgt_arg.h"

namespace kas::z80
{

// Declare argument "modes"
enum z80_arg_mode : uint8_t
{
// Standard Modes
      MODE_NONE             // 0 when parsed: indicates missing arg: always zero
    , MODE_ERROR            // 1 set error message
    , MODE_DIRECT           // 2 direct address (Z80: also accepted for immediate arg. sigh)
    , MODE_INDIRECT         // 3 indirect address
    , MODE_IMMEDIATE        // 4 immediate arg (signed byte/word)
    , MODE_IMMED_QUICK      // 5 immediate arg (stored in opcode)
    , MODE_REG              // 6 register
    , MODE_REG_INDIR        // 7 register indirect
    , MODE_REG_OFFSET       // 8 register + offset (indirect)
    , MODE_REGSET           // 9 register-set 

// Add "modes" for IX/IY as many modes (32) available & only two Index registers
// "Modes" are stored directly when args serialized. Allows prefix to be reconstructed
    , MODE_REG_IX = 16      // 16
    , MODE_REG_IY           // 17
    , MODE_REG_INDIR_IX     // 18
    , MODE_REG_INDIR_IY     // 19
    , MODE_REG_OFFSET_IX    // 20
    , MODE_REG_OFFSET_IY    // 21

// Required enumeration
    , NUM_ARG_MODES
};

// declare `token_reg`

// `REG_T` & `REGSET_T` args also allow `MCODE_T` to lookup types
struct z80_arg_t : tgt::tgt_arg_t<z80_arg_t, z80_arg_mode, void, z80_reg_t, z80_reg_set_t>
{
    // inherit basic ctors
    using base_t::base_t;
    //using error_msg = typename base_t::error_msg;
    
    // XXX slated for removal
    using arg_writeback_t = std::uint16_t *;
    

    // declare size of immed args
    static constexpr tgt::tgt_immed_info sz_info [] =
        {
              {  2 }        // 0: WORD
            , {  1 }        // 1: BYTE
        };

    // special processing for `IX`, `IY`
//XXX    void emit(core::emit_base& base, uint8_t sz, unsigned bytes) const;
    const char *set_mode(unsigned mode);

    template <typename OS> void print(OS&) const;
    
    // manage the "prefix"
    static void reset()
    { 
        prefix     = {};
        has_prefix = {};
    }

    // these are static because only 1 prefix allowed per instruction
    // NB: HL can be a "prefix" register with zero prefix code, thus two bools
    static inline uint8_t prefix;
    static inline bool    has_prefix;
};

}

#endif
