// NB: this file included several times, so no header guard

// Defintions in this file are derived from:
//
// DWARF Debugging Information Format, Version 4, Dated June 10, 2010
// http://dwarfstd.org/doc/DWARF4.pdf

// Declare features which control assembly

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

// Internal commands to facilitate frame generation
// NB: `code` is ignored. Args processed outside standard facilities
DWARF_CALL_FRAME(startproc          , 0xc1)
DWARF_CALL_FRAME(endproc            , 0xc2)

// Major commands use 2 msbs
DWARF_CALL_FRAME (advance_loc       , 0x40, BYTE)       // low6 = delta (factored)
DWARF_CALL_FRAME (offset            , 0x80, ULEB, FLEB) // low6 = register, offset
DWARF_CALL_FRAME (restore           , 0xc0, ULEB)       // low6 = register

// Normal commands are less than 64
// NB: `_sf` versions (with FLEB args) are factored versions of base command
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
DWARF_CALL_FRAME (offset_extended_sf, 17, ULEB, FLEB)   // register, offset
DWARF_CALL_FRAME (def_cfa_sf        , 18, ULEB, FLEB)   // register, offset
DWARF_CALL_FRAME (def_cfa_offset_sf , 19, FLEB)         // offset (factored)
// XXX Dwarf4, Section 6.4.2.3 6) says `val_offset` is factored, but 7) is _sf 
DWARF_CALL_FRAME (val_offset        , 20, ULEB, ULEB)   // ULEB, ULEB (factored)
DWARF_CALL_FRAME (val_offset_sf     , 21, ULEB, FLEB)   // ULEB, SLEB
DWARF_CALL_FRAME (val_expression    , 22, ULEB, BLOCK)  // ULEB, BLOCK

//
// END
//  

// clear for next run
#undef DWARF_LINE_STATE
#undef DWARF_CALL_FRAME

