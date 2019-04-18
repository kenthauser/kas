#ifndef KAS_TARGET_TGT_MCODE_DEFN_TRAIT_H
#define KAS_TARGET_TGT_MCODE_DEFN_TRAIT_H

// Instruction definion patterns
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

#include "tgt_mcode_sizes.h"         // need `sz_void`

#include <meta/meta.hpp>            // MPL library

namespace kas::tgt::opc::traits
{


///////////////////////////////////////////////////////////////////////    
//
// NB: `defn` is a meta `trait` (which evaluates to a meta `list` of arguments) because
// the `defn_flatten` metafunction recurses through each list it finds looking for more
// `defn`. This is useful because many metafunctions (eg: shift and floating point) generate
// many closely related `insns`. Since `defn` is *not* a `meta::list`, it stops the recursion.
//
///////////////////////////////////////////////////////////////////////    
// The `OP` alias is used for to allow specification of the TST & SIZE_FN arguments.
// NB: default SIZE_FN passed as void, as `tgt_defn_sizes` is needed for access
// to target `mcode_t`. Correct in `defn`
template <std::size_t OPCODE, typename TST = void, typename SIZE_FN = void>
struct OP
{
    using type    = OP;

    using code    = std::integral_constant<std::size_t, OPCODE>;
    using size_fn = SIZE_FN;
    
    // default tst "value" is always zero. Approximate
    using tst     = meta::if_<std::is_void<TST>, meta::int_<0>, TST>;
};

// NAME the INDEXES into the `meta::list` where types are located
// NB: this is only for reference. The list
// is passed as a whole to the ctor, so any changes
// in defn must also be reflected there.
static constexpr auto DEFN_IDX_SZ   = 0;
static constexpr auto DEFN_IDX_NAME = 1;
static constexpr auto DEFN_IDX_CODE = 2;
static constexpr auto DEFN_IDX_TST  = 3;
static constexpr auto DEFN_IDX_INFO = 4;    // code size function
static constexpr auto DEFN_IDX_FMT  = 5;
static constexpr auto DEFN_IDX_VAL  = 6;

// general definition: SZ list, NAME type, OP type, optional FMT & VALIDATORS
template <typename SZ, typename NAME, typename OP_INFO, typename FMT = void, typename...Ts>
struct insn_defn
{
    // six fixed types, plus additional `VALIDATORs`
    using type = meta::list<SZ
                          , NAME
                          , typename OP_INFO::code
                          , typename OP_INFO::tst
                          , typename OP_INFO::size_fn
                          , FMT             // formatter
                          , Ts...           // validators
                          >;
};

#if 0
// define specializations to default TGT & SIZE_FN: CODE holds BINARY code
template <typename SZ, typename NAME, std::size_t CODE, typename...Ts>
struct defn : insn_defn<SZ, NAME, OP<CODE>, Ts...> {};

template <typename SZ, typename NAME, typename CODE, typename...Ts>
struct defn<SZ, NAME, CODE::code, Ts...> : insn_defn<SZ, NAME, CODE, Ts...> {};
#else

template <typename...Ts>
using defn   = insn_defn<Ts...>;

template <typename...Ts>
using x_defn = insn_defn<sz_void, Ts...>;

#endif
#if 0

// define alias which omits size (default as `sz_void`)
template <typename...Ts> 
struct v_defn : defn<sz_void, Ts...> {};

//template <typename NAME, std::size_t CODE, typename...Ts>
//struct v_defn<NAME, meta::size_t<CODE>, Ts...> : v_defn<NAME, OP<CODE>, Ts...> {};
#endif
}


#endif

