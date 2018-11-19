#ifndef KAS_M68K_REG_MODES_H
#define KAS_M68K_REG_MODES_H


5/13/2016

size: Byte/Word/Long

Gen: 00/01/10  << 6
Alt: 00/01/10  << 9 (CMP2)
Alt: 01/10/11  << 9 (first word) (CAS)
Alt: 00/11/10 << 12 (Move)
Alt: xx/0 /1  << 10 (second word) MULU.[WL] (reg pair for L)
Alt: xx/11/10 <<  7 (long 020 only)
Alt: None


where they go:
mode/register at bottom of first word
4-bit register at bottom of first word (RTM)
4-bit register at top of second word (CMP2)
4-bit register at top of second & third words (CAS2)
3-bit register shifted 9 (BCHG) (MOVE)
3-bit register shifted 0 (MOVEP, LINK)
4-bit register at top of second word (Data only) (MULU)
3 bit register at bottom of second word (MULU)


cas.l 	d0,d1,GLOBAL
cas2.w 	d0:d1, d2:d3, a1:d4


From old assembler (where they go..)
 * g = general address
 * G = general address (for 32-bit instruction)
 * d = register shifted nine.
 * m = register shifted 6 (to the second `mode' position)
 * D = destination address for move (mode/reg swapped)
 * 0 = shifted zero
 * 4 = shifted four
 * 7 = shifted seven
 * 9 = shifted nine
 * A = shifted ten
 * C = shifted twelve
 *
 * b = immed. byte
 * w = immed. word
 * l = immed. long
 * I = immed format based on inst. size attribute (incl. fp)
 * W = immed. word, constant data (for movem)
 * o = branch offset -- byte/word/long
 * O = branch offset -- word only
 * B = special for bitfield instructions
 * v = special for 'div[su]l' & 'mul[su]l' (register pair shifted 12/0)
 * V = special for 'div[su]l' (single register shifted 12/0)
 * p = special for 'cas2'
 * z = verify only
 *
 * f = register shifted 7 & 10 (for float)
 * F = pair shifted 7 & 0 (for fsincos)
 * P = cp branch -- word/long

// 0 = data register direct
// 1 = addr register direct
// 2 = addr register indirect
// 3 = addr register indirect postincrement
// 4 = addr register indirect preincrement
// 5 = addr register indirect w/ displacement
// 6 = addr register indirect w/ displacement & scaled index
// 7 / 2 = pc indirect with displacement
// 7 / 3 = pc indirect with displacement & scaled index
// 7 / 0 = absolute short
// 7 / 1 = absolute long
// 7 / 4 = immediate data (2 or 4 words)

// 0: d0
// 1: a1
// 2: a2@
// 3: a3@+
// 4: a4@-
// 5: a2@(-4)
// 6: a3@(d2:4, 6:w)
// 7:

Brief:
3: Index Reg Number
1: D/A : data/address

1: W/L : index size
8: Displacement


Full extension:
3: Index Reg Number
1: D/A : data/address

1: W/L : index size

2: Scale: 0-3 = 1/2/4/8
1: Base Suppress: 1 = base suppress
2: Base Displacement Size : 01 = null, 10 = word, 11 = long
1: Index Suppress:
3: Index/Indirect Select

IS IDX
0 000 No Memory Indirect
0 001 Preindexed w/ Null Outer
0 010 Preindexed w/ Word Outer
0 011 Preindexed w/ Long Outer

0 100 Reserved
0 101 Postindexed w/ Null Outer
0 110 Postindexed w/ Word Outer
0 111 Postindexed w/ Long Outer

1 000 No Memory Indirect
1 001 Memory Indirect w/ Null Outer
1 010 Memory Indirect w/ Word Outer
1 011 Memory Indirect w/ Long Outer

1 1xx Reserved

addr indirect with 16-bit displacement
base format
mode 5: addr + 16bit ->

addr indirect with index + 8-bit displacement
brief extension
mode 6: addr + 8bit + X:size * scale ->

addr indirect index (base displacement)
full extension
mode 6: addr + 32bit base + X:size * scale ->

memory indirect postindexed
full extension
mode 6: addr + 32bit base -> X:size * scale + 32bit outer ->

memory indirect preindexed
full extension
mode 6: addr + 32bit base + X:size * scale -> 32bit outer ->

program counter indirect w/ displacement
base format
mode 7-2: pc + 16bit ->

program counter indirect with index + 8-bit displacement
brief extension
mode 7-3: pc + 8bit + X:size ->

program counter indirect with index + base displacement
full format
mode 7-3: pc + displacement + X:size * scale ->

program counter indirect postindexed
full format
mode 7-3: pc + 32bit base -> X:size * scale + 32bit outer ->

program counter indirect preindexed
full format
mode 7-3: pc + 32bit displacement + X:size * scale -> 32bit outer ->





struct reg_mode_0




#endif