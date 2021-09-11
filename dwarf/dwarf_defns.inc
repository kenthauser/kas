// NB: this file included several times, so no header guard


// Declare features which control assembly

// NOP sections not currently being processed 
#ifndef DWARF_LINE_STATE
#define DWARF_LINE_STATE(...)
#endif

#ifndef DWARF_CALL_FRAME
#define DWARF_CALL_FRAME(...)
#endif

//
// Dwarf Line
//

// declare dwarf line state variables
// NB: the order of definition *must* match order in `FSM` table
// NB: dwarf line argument types are defined in the FSM
DWARF_LINE_STATE (address)
DWARF_LINE_STATE (line)
DWARF_LINE_STATE (file)
DWARF_LINE_STATE (column)
DWARF_LINE_STATE (op_index)
DWARF_LINE_STATE (is_stmt)
DWARF_LINE_STATE (basic_block)
DWARF_LINE_STATE (end_sequence)
DWARF_LINE_STATE (prologue_end)
DWARF_LINE_STATE (epilogue_begin)
DWARF_LINE_STATE (isa)
DWARF_LINE_STATE (discriminator)


//
// Dwarf Call Frame
//

// declare dwarf call frame instructions
// The arguments are: "name", code, operand...

// Major codes use 2 msbs
DWARF_CALL_FRAME (advance_loc       , 0x40)             // low6 = delta (factored)
DWARF_CALL_FRAME (offset            , 0x80, ULEB)       // low6 = register, offset
DWARF_CALL_FRAME (restore           , 0xc0)             // low6 = register

// Normal codes are less than 64
// NB: `_sf` versions (with SLEB args) are factored versions of base command
DWARF_CALL_FRAME (nop               ,  0)               // Padding instruction
DWARF_CALL_FRAME (set_loc           ,  1, ADDR)         // relocatable address
DWARF_CALL_FRAME (advance_loc1      ,  2, BYTE)         // (factored)
DWARF_CALL_FRAME (advance_loc2      ,  3, WORD)         // (factored)
DWARF_CALL_FRAME (advance_loc4      ,  4, LONG)         // (factored)
DWARF_CALL_FRAME (offset_extended   ,  5, ULEB, ULEB)   // register, offset 
DWARF_CALL_FRAME (restore_extended  ,  6, ULEB)         // register
DWARF_CALL_FRAME (undefined         ,  7, ULEB)         // register
DWARF_CALL_FRAME (same_value        ,  8, ULEB)         // register
DWARF_CALL_FRAME (register          ,  9, ULEB, ULEB)   // register, register
DWARF_CALL_FRAME (remember_state    , 10)
DWARF_CALL_FRAME (restore_state     , 11)
DWARF_CALL_FRAME (def_cfa           , 12, ULEB, ULEB)   // register, offset
DWARF_CALL_FRAME (def_cfa_register  , 13, ULEB)         // register
DWARF_CALL_FRAME (def_cfa_offset    , 14, ULEB)         // offset (non-factored)
DWARF_CALL_FRAME (def_cfa_expression, 15, BLOCK)        // BLOCK
DWARF_CALL_FRAME (expression        , 16, ULEB, BLOCK)  // register, BLOCK
DWARF_CALL_FRAME (offset_extended_sf, 17, ULEB, SLEB)   // register, offset
DWARF_CALL_FRAME (def_cfa_sf        , 18, ULEB, SLEB)   // register, offset
DWARF_CALL_FRAME (def_cfa_offset_sf , 19, SLEB)         // offset (factored)
DWARF_CALL_FRAME (val_offset        , 20, ULEB, ULEB)   // ULEB, ULEB
DWARF_CALL_FRAME (val_offset_sf     , 21, ULEB, SLEB)   // ULEB, SLEB
DWARF_CALL_FRAME (val_expression    , 22, ULEB, BLOCK)  // ULEB, BLOCK

//
// END
//  

// clear for next run
#undef DWARF_LINE_STATE
#undef DWARF_CALL_FRAME

