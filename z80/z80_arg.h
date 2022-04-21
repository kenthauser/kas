#ifndef KAS_Z80_Z80_ARG_H
#define KAS_Z80_Z80_ARG_H

// Declare z80 argument & arg MODES

#include "z80_reg_types.h"
#include "target/tgt_arg.h"

namespace kas::z80
{

// Declare argument "modes"
// These must be "defined" as they are referenced in `target/*` code.
// The `enum` values need not be in any preset order.
// The `NUM_ARG_MODES` is the highest value in use. Required enums may
// have values above `NUM_ARG_MODES` if not used
enum z80_arg_mode : uint8_t
{
// Standard Modes
      MODE_NONE             //  0 when parsed: indicates missing arg
    , MODE_ERROR            //  1 set error message
    , MODE_DIRECT           //  2 direct address (Z80: allaccepted for immed arg. sigh.)
    , MODE_INDIRECT         //  3 indirect address
    , MODE_IMMEDIATE        //  4 immediate arg (signed byte/word)
    , MODE_IMMED_QUICK      //  5 immediate arg (stored in opcode)
    , MODE_REG              //  6 register
    , MODE_REG_INDIR        //  7 register indirect
    , MODE_REG_P_OFFSET     //  8 register + offset (indirect)
    , MODE_REG_M_OFFSET     //  9 register + offset (indirect)
    , MODE_REGSET           // 10 register-set 

// Add "modes" for IX/IY as many modes (32) available & only two Index registers
// "Modes" are stored directly when args serialized. Allows prefix to be reconstructed
   , MODE_REG_IX = 16       // 16
   , MODE_REG_IY            // 17
   , MODE_REG_INDIR_IX      // 18
   , MODE_REG_INDIR_IY      // 19
   , MODE_REG_P_OFFSET_IX   // 20   IX with PLUS offset
   , MODE_REG_P_OFFSET_IY   // 21   IY with PLUS offset
   , MODE_REG_M_OFFSET_IX   // 22   IX with MINUS offset
   , MODE_REG_M_OFFSET_IY   // 23   IY with MINUS offset



// Required enumerations
    , NUM_ARG_MODES
    , MODE_BRANCH           // relative branch size (Z80, always byte)
    , MODE_BRANCH_LAST      // only 1 branch insn
};

// `REG_T` & `REGSET_T` args also allow `MCODE_T` to lookup types
struct z80_arg_t : tgt::tgt_arg_t<z80_arg_t
                                 , z80_arg_mode
                                 , z80_reg_t
                                 , z80_reg_set_t>
{
    // inherit basic ctors
    using base_t::base_t;

    // declare size of immed args: indexed on `z80_op_size_t`
    static constexpr tgt::tgt_immed_info sz_info [] =
        {
              {  2 }        // 0: OP_SIZE_WORD
            , {  1 }        // 1: OP_SIZE_BYTE
            , {  2 }        // 2: OP_SIZE_VOID  (eg call, branch)
        };

   // special processing for `IX`, `IY`
   const char *set_mode(unsigned mode);

   // calculate size of extension data for argument (based on MODE & reg/expr values)
   int size(uint8_t sz, expression::expr_fits const *fits_p = {}, bool *is_signed = {}) const;
   
   // customize emit for IX/IY
   void emit      (core::core_emit& base, uint8_t sz) const;

   template <typename OS> void print(OS&) const;
   
   // clear "per-insn" arg (ie static) data: manage the "prefix"
   static void reset()
   { 
       prefix     = {};
       has_prefix = {};
   }

   // these are static because only 1 prefix allowed per instruction
   // NB: HL can be a "prefix" register with zero prefix code, thus two values. 
   // eg: "add ix,ix" & "add hl,hl" allowed. but "add ix,hl" not allowed
   static inline uint8_t prefix;
   static inline bool    has_prefix;
};

}

#endif
