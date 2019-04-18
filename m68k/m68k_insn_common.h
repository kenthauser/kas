#ifndef KAS_M68K_M68K_INSN_COMMON_H
#define KAS_M68K_M68K_INSN_COMMON_H

// m68k instruction definion patterns
//
// The m68k instructions are defined as a sequence of types.
//
// Example:
//  insn<sz_lwb, STR("move"), OP<0x0000, void, INFO_SIZE_MOVE>, FMT_MOVE_0RM_6RM, GEN, ALTERABLE>
//
// Breaking down the fields,
//  - the first "type" describes which "sizes" to apply to the rest of the instruction
//  - the second "type" is the base name" of the instruction
//  - the third type holds the "opcode" base value, a `TST` function, and instructions
//      on how to encode the "size" within the "opcode" base value
//  - the fourth field holds a type which describes how to insert & remove "arguments" in
//      the base opcode
//  - the remaining fields are a varadic list of "validators", one for each argument.
//
// Thus in the example, "long, word, and byte" (sz_lwb) variants of the "move" instruction
// are generated. The size field of the instruction is inserted into the base value of "0000"
// by `INSN_SIZE_MOVE`. After validating two arguments using validators "GEN" and "ALTERABLE"
// the resulting arguments are inserted into the opcode using "FMT_MOVE_0RM_6RM"
//
// The naming patterns for the `FMT_*` & `VALIDATORS` types are described in their header files.
//
// Finally the value of the base `opcode` holds a zero for the "size" field if more than
// one size is generated. If only a single size is generated (even if it has multiple names),
// the `opcode` holds the value shown in the databook, and the SIZE_FN is not applied.
//
//////////////////////////////////////////////////////////////////////////////////////////
//
// The `insn` definition six constexpr arrays linked by indexes. 
//
// The definition instances are in an array of `m68k_insn_defn` which holds indexs 
// into `name`, `sz`, `info`, `format` and `validator_combo` constexpr arrays.
// 
// These `constexpr arrays` are created by`parser::sym_parser_t`
//
//////////////////////////////////////////////////////////////////////////////////////////

#include "m68k_size_lwb.h"          // types to insert "size" into opcode
#include "m68k_formats_defn.h"      // actual format types
#include "m68k_validate_reg.h"      // actual validate types
#include "m68k_validate_gen.h"      // actual validate types

#include "target/tgt_mcode_defn.h"   // declare constexpr defn
#include "target/tgt_mcode_defn_trait.h"  // decalare "trait" for definition
#include "kas/kas_string.h"         // name as type

namespace kas::m68k::opc
{
// declare opcode groups (ie: include files)
using m68k_defn_groups = meta::list<
      struct OP_M68K_GEN
    , struct OP_M68K_020
    , struct OP_M68K_040
    , struct OP_M68K_060
    , struct OP_M68K_CPU32
    , struct OP_M68K_68881
    , struct OP_M68K_68551
    , struct OP_COLDFIRE
    >;

template <typename=void> struct m68k_defn_list : meta::list<> {};

//
// declare "size traits" for use in instruction definintion
//

using namespace tgt::opc::traits;
using tgt::opc::define_sz;
using tgt::opc::sz_void;

// multiple sizes: generate `list` directly`
using sz_lwb  = meta::list<meta::int_<(1 << OP_SIZE_LONG) | (1 << OP_SIZE_WORD) | (1 << OP_SIZE_BYTE)>>;
using sz_lw   = meta::list<meta::int_<(1 << OP_SIZE_LONG) | (1 << OP_SIZE_WORD)>>;
using sz_wb   = meta::list<meta::int_<(1 << OP_SIZE_WORD) | (1 << OP_SIZE_BYTE)>>;
using sz_all  = meta::list<meta::int_<0x7f>>;

// single-sizes
using sz_b    = define_sz<OP_SIZE_BYTE>;
using sz_w    = define_sz<OP_SIZE_WORD>;
using sz_l    = define_sz<OP_SIZE_LONG>;
using sz_s    = define_sz<OP_SIZE_SINGLE>;
using sz_d    = define_sz<OP_SIZE_DOUBLE>;
using sz_x    = define_sz<OP_SIZE_XTND>;
using sz_p    = define_sz<OP_SIZE_PACKED>;

// void never has suffix (also: always single size)
using sz_v    = sz_void;

// only difference between v% & %v: first name is canonical.
using sz_wv   = define_sz<OP_SIZE_WORD, tgt::opc::SFX_OPTIONAL>;
using sz_vw   = define_sz<OP_SIZE_WORD, tgt::opc::SFX_CANONICAL_NONE>;
using sz_lv   = define_sz<OP_SIZE_LONG, tgt::opc::SFX_OPTIONAL>;
using sz_vl   = define_sz<OP_SIZE_LONG, tgt::opc::SFX_CANONICAL_NONE>;
using sz_bv   = define_sz<OP_SIZE_BYTE, tgt::opc::SFX_OPTIONAL>;
using sz_vb   = define_sz<OP_SIZE_BYTE, tgt::opc::SFX_CANONICAL_NONE>;

// set size field, but no suffix (capital W/L). not common.
using sz_W    = define_sz<OP_SIZE_WORD, tgt::opc::SFX_NONE>;
using sz_L    = define_sz<OP_SIZE_LONG, tgt::opc::SFX_NONE>;

}


#endif

