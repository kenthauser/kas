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
// The arguments are: "name", high 2 bits, low 6 bits, arg_cnt, operand 1, operand 2

// row creation instructions
DWARF_CALL_FRAME (set_loc           , 0, 0x01, 1, ADDR)        // relocatable address
DWARF_CALL_FRAME (advance_loc       , 1, 0,    0)              // low6 = delta (factored)
DWARF_CALL_FRAME (advance_loc1      , 0, 0x02, 1, BYTE)        // (factored)
DWARF_CALL_FRAME (advance_loc2      , 0, 0x03, 1, WORD)        // (factored)
DWARF_CALL_FRAME (advance_loc4      , 0, 0x04, 1, LONG)        // (factored)

// CFA definition instructions
DWARF_CALL_FRAME (def_cfa           , 0, 0x0c, 2, ULEB, ULEB)  // register, offset (non-factored)
DWARF_CALL_FRAME (def_cfa_sf        , 0, 0x12, 2, ULEB, SLEB)  // register, offset
DWARF_CALL_FRAME (def_cfa_register  , 0, 0x0d, 1, UELB)        // register
DWARF_CALL_FRAME (def_cfa_offset    , 0, 0x0e, 1, ULEB)        // offset (non-factored)
DWARF_CALL_FRAME (def_cfa_offset_sf , 0, 0x13, 1, SLEB)        // register
DWARF_CALL_FRAME (def_cfa_expression, 0, 0x0f, 1, BLOCK)       // BLOCK

// Register rule instructions
DWARF_CALL_FRAME (undefined         , 0, 0x07, 1, ULEB)        // register
DWARF_CALL_FRAME (same_value        , 0, 0x08, 1, ULEB)        // register
DWARF_CALL_FRAME (offset            , 2, 0   , 1, ULEB)        // low6 = register, offset
DWARF_CALL_FRAME (offset_extended   , 0, 0x05, 2, ULEB, ULEB)  // register, offset 
DWARF_CALL_FRAME (offset_extended_sf, 0, 0x11, 2, ULEB, SLEB)  // register, offset
DWARF_CALL_FRAME (val_offset        , 0, 0x14, 2, ULEB, ULEB)  // ULEB, ULEB
DWARF_CALL_FRAME (val_offset_sf     , 0, 0x15, 2, ULEB, SLEB)  // ULEB, SLEB 
DWARF_CALL_FRAME (register          , 0, 0x09, 2, ULEB, ULEB)  // register, register
DWARF_CALL_FRAME (expression        , 0, 0x10, 2, ULEB, BLOCK) // register, BLOCK
DWARF_CALL_FRAME (val_expression    , 0, 0x16, 2, ULEB, BLOCK) // ULEB, BLOCK
DWARF_CALL_FRAME (restore           , 3, 0,    0)              // low6 = register
DWARF_CALL_FRAME (restore_extended  , 0, 0x06, 1, ULEB)        // register

// Row state instructions
DWARF_CALL_FRAME (remember_state    , 0, 0x0a, 0)
DWARF_CALL_FRAME (restore_state     , 0, 0x0b, 0)

// Padding instruction
DWARF_CALL_FRAME (nop               , 0, 0,    0)

//
// END
//  

// clear for next run
#undef DWARF_LINE_STATE
#undef DWARF_CALL_FRAME

