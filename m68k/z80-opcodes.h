/* opcode table generation for z80
 *
 * $Id: z80-opcodes.h,v 1.1 1991/05/23 16:10:06 kent Exp $
 *
 * Copyright (c) 1989, Kent Hauser
 *
 * This file is part of AS, a retargetable assembler.
 *
 * AS is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, Inc., Version 1.
 */

/* We store two bytes of opcode for all opcodes because that
   is the most any of them need.  The actual length of an instruction
   is always at least 1 byte, and is as much longer as necessary to
   hold the operands it has.

   The args component is a string containing two characters
   for each operand of the instruction.  The first specifies
   the kind of operand; the second, the place it is stored.  */

/* Kinds of operands:
   r  8-bit "register"         A,B,C,D,E,H,L,(HL),(IX+d),(IY+d)
   d  16-bit dbl-add register  BC, DE, SP
   s  16-bit register          BC, DE, SP, HL
   q  16-bit push register     AF, BC, DE, HL, IX, IY
   h  16-bit accum register    HL, IX, IY
   @  16-bit indir register    (HL), (IX), (IY)
   A  A-register
   B  BC-register, or DE-register, indirect: (BC), (DE)
   D  DE-register
   H  HL-register
   F  AF-register
   I  I-register, or R-register  I=0/R=1
   S  SP-register
   X  IX-register
   Y  IY-register
   P  the string "(C)" (or @c)
   T  (SP)-register
   c  cond-codes              Z, NZ, C, NC, PO, PE, P, M
   C  jr cond-codes           Z, NZ, C, NC

   b  byte data (8-bit)
   w  word data (16-bit)
   o  pc-relative offset
   %  immed indirect (eg ld a,(0x100))
   p  immed indirect, 8-bit
   n  bit number (0-7)
   x  restart number ( [0..7] << 3 )
   m  interrupt mode control: map as follows: 0->0, 1->2, 2->3
*/

/* Places to put an operand:
   0  first byte, shifted 0
   1  first byte, shifted 3
   2  first byte, shifted 4
   3  second byte, shifted 0
   4  second byte, shifted 3
   5  second byte, shifted 4

   i  immed.
   Z  nop, verify only

   Implied with (ix,iy) usage is prefix with (0xdd, 0xfd).  These
   prefixed are not included in the byte counts listed above.

   Also implied with the (ix+n,iy+n) format allowed with format "r"
   is inclusion of the offset byte ``n'', as the third byte of the
   expanded opcode.

   Immediate data follows the opcode, low byte first.
*/

#define	op(name, value, arg_list) \
  { {name}, arg_list, value, ((value) >= 0x100) + 1, \
      sizeof (arg_list)/2, sizeof (name) - 1 },

struct opcode   op_table[] =
{
#include "z80-opcodes.def"
    {}
};

const int       num_ops = sizeof (op_table) / sizeof (struct opcode);

#define __REG(sym)		__REGSYM (sym, sym)
#define __REGSYM(sym, value)	{ #sym, REG_ ## value, sizeof (#sym) - 1 },

static struct reg_table {
    char           *reg_name;
    enum machine_reg reg_value;
    short           name_length;
}               reg_table[] =

{
#include "z80-registers.def"
    { }
};

/*
 * Local Variables:
 * backup-by-copying-when-linked: t
 * End:
 */
