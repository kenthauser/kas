#ifndef KAS_Z80_Z80_INSN_COMMON_H
#define KAS_Z80_Z80_INSN_COMMON_H

// z80 instruction definion patterns
//
// The z80 instructions are defined as a sequence of types.
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
// The definition instances are in an array of `z80_insn_defn` which holds indexs 
// into `name`, `sz`, `info`, `format` and `validator_combo` constexpr arrays.
// 
// These `constexpr arrays` are created by`parser::sym_parser_t`
//
//////////////////////////////////////////////////////////////////////////////////////////

#include "z80_validate_reg.h"      // actual validate types
#include "z80_formats_defn.h"      // actual format types

#include "kas/kas_string.h"         // name as type

#include <meta/meta.hpp>            // MPL library

namespace kas::z80::opc
{

// declare opcode groups (ie: include files)
using z80_insn_defn_groups = meta::list<
      struct OP_Z80_GEN
    >;

template <typename=void> struct z80_insn_defn_list : meta::list<> {};


///////////////////////////////////////////////////////////////////////    
//
// NB: `insn` is a meta `trait` (which evaluates to a meta `list` of arguments) because
// the `defn_flatten` metafunction recurses through each list it finds looking for more
// `insns`. This is useful because many metafunctions (eg: shift and floating point) generate
// many closely related `insns`. Since `insn` is *not* a `meta::list`, it stops the recursion.
//
///////////////////////////////////////////////////////////////////////    

// default fmt: no args (and thus no inserter)

template <typename NAME, std::size_t CODE, typename FMT = void, typename...Ts>
struct defn
{
    // if no formatter specified, use "general" format w/o args
    using fmt  = meta::if_<std::is_void<FMT>, fmt_gen, FMT>;
    using type = meta::list<NAME, std::integral_constant<std::size_t, CODE>, fmt, Ts...>;
};

}


#endif

