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
// The definition instances are in an array of `m68k_insn_defn` which holds indexes 
// into `name`, `sz`, `info`, `format` and `validator_combo` constexpr arrays.
// 
// These `constexpr arrays` are created by`parser::sym_parser_t`
//
//////////////////////////////////////////////////////////////////////////////////////////

#include "m68k_size_lwb.h"          // types to insert "size" into opcode
#include "m68k_validate_reg.h"      // actual validate types
#include "m68k_validate_gen.h"      // actual validate types
#include "m68k_formats_defn.h"      // actual format types

#include "target/tgt_mcode_defn.h"  // declare constexpr defn
#include "target/tgt_insn_common.h" // decalare "trait" for definition
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

// XXX I can't get `fold` with integer types & shift to work
// XXX Just use constexpr function instead. KBH 2019/07/18
template <typename...BITS>
constexpr auto m68k_as_mask(unsigned value, unsigned bit, BITS...bits)
{
    value |= 1 << bit;
    if constexpr (sizeof...(BITS) != 0)
        return m68k_as_mask(value, bits...);
    return value;
}

template <typename SFX, unsigned DFLT = OP_SIZE_VOID, unsigned...SIZES>
using m68k_sz = meta::int_<m68k_as_mask(SFX::value, DFLT, SIZES...)>;

// test if `SZ` represents multiple SZs (eg not just BYTE, WORD, or LONG)
template <typename SZ, auto SZs = SZ::value & 0x7f>
constexpr bool MULTIPLE_SZ = SZs & (SZs - 1);

// instructions for suffix handling:
// eg: instructions such as `moveq.l` can also be spelled `moveq`
// SFX_* types say how to handle "blank" suffix
// ccode:       disallow T/F condition codes
// ccode_all:   allow T/F condition codes


// NB: 4 MSBs of 16-bit `SZ` used to hold `INFO_SIZE` index
using SFX_NORMAL        = meta::int_<0x0000>;    // sfx required
using SFX_OPTIONAL      = meta::int_<0x0100>;    // sfx optional
using SFX_CANON_NONE    = meta::int_<0x0200>;    // no sfx is canonical
using SFX_NONE          = meta::int_<0x0300>;    // sfx prohibited
using SFX_CCODE_BIT     = meta::int_<0x0400>;    // CCODE req'd (4 bits shifted 8)
using SFX_CCODE         = meta::int_<SFX_CCODE_BIT::value | SFX_OPTIONAL::value>;
using SFX_CCODE_ALL     = meta::int_<SFX_CCODE_BIT::value | SFX_NONE::value>;    

using SFX_CCODE_FP      = meta::int_<0x800>;     // FP ccode: first word, 5 LSBs
using SFX_CCODE_RAW     = meta::int_<0xc00>;     // raw FP code (for list): 6 bits shifted 6

static constexpr auto SFX_MASK  = 0x300;     // flag mask to test SFX codes
static constexpr auto SFX_IS_CC = 0xc00;     // flag mask to test if condition code

using sz_void   = m68k_sz<SFX_NONE>;

using sz_list = meta::int_<SFX_NORMAL::value | SFX_CCODE_RAW::value | 0xff>;

using sz_lwb = m68k_sz<SFX_NORMAL, OP_SIZE_LONG, OP_SIZE_WORD, OP_SIZE_BYTE>;
using sz_lw  = m68k_sz<SFX_NORMAL, OP_SIZE_LONG, OP_SIZE_WORD>; 
using sz_wb  = m68k_sz<SFX_NORMAL, OP_SIZE_WORD, OP_SIZE_BYTE>;
using sz_all = meta::int_<SFX_NORMAL::value | 0x7f>;        // don't generate fancy meta-code

// single-sizes
using sz_b    = m68k_sz<SFX_NORMAL, OP_SIZE_BYTE>;
using sz_w    = m68k_sz<SFX_NORMAL, OP_SIZE_WORD>;
using sz_l    = m68k_sz<SFX_NORMAL, OP_SIZE_LONG>;
using sz_s    = m68k_sz<SFX_NORMAL, OP_SIZE_SINGLE>;
using sz_d    = m68k_sz<SFX_NORMAL, OP_SIZE_DOUBLE>;
using sz_x    = m68k_sz<SFX_NORMAL, OP_SIZE_XTND>;
using sz_p    = m68k_sz<SFX_NORMAL, OP_SIZE_PACKED>;

// void never has suffix (also: always single size)
using sz_v    = m68k_sz<SFX_NONE>;

// only difference between v% & %v: first name is canonical.
using sz_wv   = m68k_sz<SFX_OPTIONAL    , OP_SIZE_WORD>;
using sz_vw   = m68k_sz<SFX_CANON_NONE  , OP_SIZE_WORD>;
using sz_lv   = m68k_sz<SFX_OPTIONAL    , OP_SIZE_LONG>;
using sz_vl   = m68k_sz<SFX_CANON_NONE  , OP_SIZE_LONG>;
using sz_bv   = m68k_sz<SFX_OPTIONAL    , OP_SIZE_BYTE>;
using sz_vb   = m68k_sz<SFX_CANON_NONE  , OP_SIZE_BYTE>;

// set size field, but no suffix (capital W/L). not common.
using sz_W    = m68k_sz<SFX_NONE        , OP_SIZE_WORD>;
using sz_L    = m68k_sz<SFX_NONE        , OP_SIZE_LONG>;

// condition code instructions
using sz_cc     = m68k_sz<SFX_CCODE>;
using sz_cc_all = m68k_sz<SFX_CCODE_ALL>;
using sz_cc_fp  = meta::int_<SFX_CCODE_FP::value | 0x7f>;

//
// Override `tgt_insn_common` metafunctions to accommodate the M68K `SIZE_FN`
//
// `SIZE_FN` value is merged into `SZ` value
//

template <std::size_t OPCODE, typename TST = void, typename SIZE_FN = void, std::size_t MASK = 0>
struct OP
{
    using type    = OP;

    using code    = std::integral_constant<std::size_t, OPCODE>;
    using mask    = std::integral_constant<std::size_t, MASK>;

    // default `SIZE_FN` index to zero
    using size_fn_idx = meta::if_<std::is_void<SIZE_FN>
                            , meta::int_<0>
                            , meta::find_index<LWB_SIZE_LIST, SIZE_FN>
                            >;

    static_assert(!std::is_same_v<size_fn_idx, meta::npos>, "Invalid SIZE_FN");
    
    // default tst "value" is always zero. Approximate default test
    using tst     = meta::if_<std::is_void<TST>, meta::int_<0>, TST>;
};

// general definition: SZ list, NAME type, OP type, optional FMT & VALIDATORS
template <typename SZ, typename NAME, typename OP_INFO, typename FMT = void, typename...Ts>
struct defn
{
    // default "size" is `int_<0>`
    using DEFN_INFO = meta::if_<std::is_same<void, SZ>, meta::int_<0>, SZ>;

    // select IDX_NONE if single type in SZ
    using INFO_IDX = std::conditional_t<MULTIPLE_SZ<SZ>
                                    , typename OP_INFO::size_fn_idx
                                    , meta::int_<1>
                                    >;

    // M68K_INFO picks up additional value from `OP`
    using M68K_INFO = meta::int_<DEFN_INFO::value | (INFO_IDX::value << 12)>;

    // six fixed types, plus additional `VALIDATORs`
    using type = meta::list<M68K_INFO
                          , NAME
                          , typename OP_INFO::code
                          , typename OP_INFO::tst
                          , FMT             // formatter
                          , Ts...           // validators
                          >;
};

}


#endif

