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

#include "m68k_size_defn.h"         // types defining "size" list for opcodes
#include "m68k_size_lwb.h"          // types to insert "size" into opcode
#include "m68k_validate_reg.h"      // actual validate types
#include "m68k_validate_gen.h"      // actual validate types
#include "m68k_formats_defn.h"      // actual format types

#include "kas/kas_string.h"         // name as type

#include <meta/meta.hpp>            // MPL library

namespace kas::m68k::opc
{
///////////////////////////////////////////////////////////////////////    
//
// NB: `insn` is a meta `trait` (which evaluates to a meta `list` of arguments) because
// the `defn_flatten` metafunction recurses through each list it finds looking for more
// `insns`. This is useful because many metafunctions (eg: shift and floating point) generate
// many closely related `insns`. Since `insn` is *not* a `meta::list`, it stops the recursion.
//
///////////////////////////////////////////////////////////////////////    

// default fmt: no args (and thus no inserter)

template <typename SZ, typename NAME, typename INFO, typename FMT = void, typename...Ts>
struct insn
{
    // if no formatter specified, use "general" format w/o args
    using fmt  = meta::if_<std::is_void<FMT>, fmt_gen, FMT>;
    using type = meta::list<SZ, NAME, INFO, fmt, Ts...>;
};

// The `OP` alias is used for `INFO` to allow defaults for the TST & SIZE_FN arguments.
template <uint32_t OPCODE, typename TST = void, typename SIZE_FN = INFO_SIZE_NORM>
struct OP
{
    using type    = OP;

    using opcode  = std::integral_constant<uint32_t, OPCODE>;
    using tst     = meta::if_<std::is_void<TST>, hw::hw_void, TST>;
    using size_fn = meta::_t<SIZE_FN>;
};

}


#endif

