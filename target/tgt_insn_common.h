#ifndef KAS_TARGET_TGT_INSN_COMMON_H
#define KAS_TARGET_TGT_INSN_COMMON_H

// Instruction definion patterns
//
// The KAS target instructions are defined as a sequence of types.
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
// The definition instances are in an array of `tgt_mcode_defn_defn` which holds indexs 
// into constexpr arrays as well as integral defns.
// 
// These `constexpr arrays` are created by`parser::sym_parser_t`
//
//////////////////////////////////////////////////////////////////////////////////////////

#include "tgt_mcode_sizes.h"        // need `sz_void`
#include "tgt_mcode_defn.h"         // declare constexpr defn
#include "kas/kas_string.h"         // name as type

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
template <std::size_t OPCODE, typename TST = void, typename INFO_FN = void, std::size_t MASK = 0>
struct OP
{
    // expose template args as member types
    using type    = OP;
    using info_fn = INFO_FN;
    using tst     = TST;

    // wrap integral vaues in `size_t` types
    using code    = std::integral_constant<std::size_t, OPCODE>;
    using mask    = std::integral_constant<std::size_t, MASK>;
};

// add value to OP
template <typename OP, unsigned N, unsigned SHIFT>
struct OP_ADD : OP
{
    static constexpr auto value = OP::code + (N << SHIFT);
    using code = std::integral_constant<std::size_t, value>;
};


// NAME the INDEXES into the `meta::list` where types are located
// NB: this is only for reference. The list
// is passed as a whole to the ctor, so any changes
// in defn must also be reflected there.
static constexpr auto DEFN_IDX_SZ      = 0;
static constexpr auto DEFN_IDX_INFO_FN = 1;
static constexpr auto DEFN_IDX_NAME    = 2;
static constexpr auto DEFN_IDX_CODE    = 3;
static constexpr auto DEFN_IDX_TST     = 4;
static constexpr auto DEFN_IDX_FMT     = 5;
static constexpr auto DEFN_IDX_VAL     = 6;

// general definition: INFO_FLAGS type, NAME type, OP type, optional FMT & VALIDATORS
template <typename MCODE_T, typename INFO_MAP, typename INFO_FLAGS
        , typename NAME, typename OP, typename FMT = void, typename...VALs>
struct tgt_insn_defn
{
    // provide default for INFO_MAP (if defined as void)
    using X_INFO_MAP = meta::if_<std::is_void<INFO_MAP>
                               , meta::pair<void, void>
                               , INFO_MAP>;

    // prep the info-fn map values
    using ZIPPED    = meta::zip<INFO_MAP>;
    
    // see if INFO_FLAGS is in the info-fn map
    using FLAGS_IDX = meta::find_index<meta::front<ZIPPED>, INFO_FLAGS>;

    // extract INFO_FN if INFO_FLAGS in map
    // if `OP::info_fn` specified, don't override
    // default to `info_fn_t` if not specified
    using DFLT_INFO_FN = meta::if_<std::is_void<typename OP::info_fn>
                                 , typename MCODE_T::info_fn_t
                                 , typename OP::info_fn
                                 >;

    // if FN not specified in `OP` and the FLAGS in in `map`, retrieve appropriate `INFO_FN`
    // NB: need to defer evaluation (prevent `at` from possibly being evaluated with `npos`)
    using INFO_FN = std::conditional_t<
                                    !std::is_void_v<typename OP::info_fn> ||
                                    std::is_same_v<FLAGS_IDX, meta::npos>
                                  , meta::id<DFLT_INFO_FN>
                                  , meta::lazy::at<meta::back<ZIPPED>, FLAGS_IDX>
                                  >;
   
    // *** CLEAN UP VALUES *** 
    // NB: `meta::if_` is less typing, not better than, `std::conditional_t`

    // map `void` FLAGS to zero
    using DEFN_INFO_FLAGS = meta::if_<std::is_void<INFO_FLAGS>
                                    , meta::int_<0>
                                    , INFO_FLAGS
                                    >;

    // map `void` test to `int_<0>` (always matches)
    using DEFN_TST        = meta::if_<std::is_void<typename OP::tst>
                                    , meta::int_<0>
                                    , typename OP::tst
                                    >;
   
    // six fixed types, plus additional `VALIDATORs`
    using type = meta::list<DEFN_INFO_FLAGS     // rationalized FLAGS
                          , meta::_t<INFO_FN>   // evaluate deferred calculation
                          , NAME                // opcode base NAME
                          , typename OP::code   // retrieve base binary code
                          , DEFN_TST            // rationalized TST
                          , FMT                 // formatter
                          , VALs...             // validators
                          >;
};
}


#endif

