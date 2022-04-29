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

#include "m68k_formats.h"           // arg insertion format types
#include "m68k_info_impl.h"         // insert "stmt_info" into opcode
#include "m68k_validate_reg.h"
#include "m68k_validate_gen.h"

#include "target/tgt_insn_common.h"  // declare "trait" for definition

namespace kas::m68k::opc
{

// declare opcode groups (ie: include files)
using m68k_insn_defn_groups = meta::list<
      struct OP_M68K_GEN
    , struct OP_M68K_020
    , struct OP_M68K_040
    , struct OP_M68K_060
    , struct OP_M68K_CPU32
    , struct OP_M68K_FPU
    , struct OP_M68K_MMU
    , struct OP_COLDFIRE
    >;

template <typename=void> struct m68k_insn_defn_list : meta::list<> {};

using namespace tgt::opc::traits;
//
// declare "defn_info" values
//
// The `defn_info` is a 16-bit field which augments the mcode defn.
//
// M68K opcodes can allow multiple sizes -- each instruction can
// allow a specific subset. The LSBs of the LSBs of the `defn_info`
// are a bit-mask indenpendently allowing each of the seven "sizes"
// for the mcode. bit 0x80 is set to disallow all sizes.
//
// The next 4 bits (0x0x00) are used to identify if size suffix is optional,
// required, and/or preferred (ie canonical). These bits also if a condition
// code is allowed, and if `t` and `f` condition codes are allowed.
//
// Finally the 4 MSBs are used to encoded the argument size is encoded
// in the machine code. There are 10 (!) different methods. These methods
// are specified in the OP<> during insn definition. The methods are encoded
// in `m68k_info_impl.h`

// create bitfield "mask" of allow SIZES (`m68k_op_size`) for INSN
template <typename...BITS>
constexpr auto m68k_sizes_as_mask(unsigned value, unsigned bit, BITS...bits)
{
    value |= 1 << bit;
    if constexpr (sizeof...(BITS) != 0)
        return m68k_sizes_as_mask(value, bits...);
    return value;
}

template <typename SFX, unsigned DFLT = OP_SIZE_VOID, unsigned...SIZES>
using m68k_sizes = meta::int_<m68k_sizes_as_mask(SFX::value, DFLT, SIZES...)>;

// test if `SZ` represents multiple SZs (eg not just BYTE, WORD, or LONG)
template <typename SZ, auto SZs = SZ::value & 0x7f>
constexpr bool SINGLE_SZ = !(SZs & (SZs - 1));

// instructions for suffix handling:
// eg: instructions such as `moveq.l` can also be spelled `moveq`
// SFX_* types say how to handle "blank" suffix
// ccode:       disallow T/F condition codes
// ccode_all:   allow T/F condition codes


// Generate value stored in `m68k_defn_info`
// NB: 4 MSBs of 16-bit `SZ` used to hold `INFO_SIZE` fn index
using SFX_NORMAL        = meta::int_<0x0000>;   // sfx required
using SFX_OPTIONAL      = meta::int_<0x0100>;   // sfx optional
using SFX_CANON_NONE    = meta::int_<0x0200>;   // no sfx is canonical
using SFX_NONE          = meta::int_<0x0300>;   // sfx prohibited
// ccode allowed uses two bits. only 2-of-3 values assigned
using SFX_CCODE         = meta::int_<0x0400>;   // CCODE req'd  NO T/F
using SFX_CCODE_ALL     = meta::int_<0x0c00>;   // CCODE req'd: all codes
using SFX_unassigned    = meta::int_<0x0800>;   // available code

// `arch` values are not in TMP metafunctions. just declare values
// NB: values much match those in `m6k_stmt_flags` ccode defns
static constexpr auto SFX_CPID_FPU  = 0x0400;   // FPU instruction
static constexpr auto SFX_CPID_MMU  = 0x0800;   // MMU instruction
static constexpr auto SFX_CPID_unassigned = 0xc00;


static constexpr auto SFX_SZ_MASK    = 0x00ff;  // mask to test SFX codes
static constexpr auto SFX_CANON_MASK = 0x0300;  // mask for canonical display
static constexpr auto SFX_CPID_MASK  = 0x0c00;  // mask for coprocessor insns
static constexpr auto SFX_CPID_SHIFT = 10;      // bits
 
using sz_void   = m68k_sizes<SFX_NONE>;

using sz_lwb = m68k_sizes<SFX_NORMAL, OP_SIZE_LONG, OP_SIZE_WORD, OP_SIZE_BYTE>;
using sz_lw  = m68k_sizes<SFX_NORMAL, OP_SIZE_LONG, OP_SIZE_WORD>; 
using sz_wb  = m68k_sizes<SFX_NORMAL, OP_SIZE_WORD, OP_SIZE_BYTE>;
using sz_all = meta::int_<SFX_NORMAL::value | 0x7f>;

// single-sizes
using sz_b    = m68k_sizes<SFX_NORMAL, OP_SIZE_BYTE>;
using sz_w    = m68k_sizes<SFX_NORMAL, OP_SIZE_WORD>;
using sz_l    = m68k_sizes<SFX_NORMAL, OP_SIZE_LONG>;
using sz_s    = m68k_sizes<SFX_NORMAL, OP_SIZE_SINGLE>;
using sz_d    = m68k_sizes<SFX_NORMAL, OP_SIZE_DOUBLE>;
using sz_x    = m68k_sizes<SFX_NORMAL, OP_SIZE_XTND>;
using sz_p    = m68k_sizes<SFX_NORMAL, OP_SIZE_PACKED>;

// void never has suffix (also: always single size)
using sz_v    = m68k_sizes<SFX_NONE>;

// only difference between v% & %v: first name is canonical.
using sz_wv   = m68k_sizes<SFX_OPTIONAL    , OP_SIZE_WORD>;
using sz_vw   = m68k_sizes<SFX_CANON_NONE  , OP_SIZE_WORD>;
using sz_lv   = m68k_sizes<SFX_OPTIONAL    , OP_SIZE_LONG>;
using sz_vl   = m68k_sizes<SFX_CANON_NONE  , OP_SIZE_LONG>;
using sz_bv   = m68k_sizes<SFX_OPTIONAL    , OP_SIZE_BYTE>;
using sz_vb   = m68k_sizes<SFX_CANON_NONE  , OP_SIZE_BYTE>;

// set size field, but no suffix (capital W/L). not common.
using sz_W    = m68k_sizes<SFX_NONE        , OP_SIZE_WORD>;
using sz_L    = m68k_sizes<SFX_NONE        , OP_SIZE_LONG>;
#if 0
// condition code instructions
using cc_v    = meta::int_<SFX_CCODE_ALL::value | sz_v ::value>;
using cc_vb   = meta::int_<SFX_CCODE_ALL::value | sz_vb::value>;
using cc_wv   = meta::int_<SFX_CCODE_ALL::value | sz_vw::value>;
using cc_l    = meta::int_<SFX_CCODE_ALL::value | sz_l ::value>;
using cx_v    = meta::int_<SFX_CCODE    ::value | sz_v ::value>;
#endif

using INFO_CCODE_NOTF = INFO_CCODE_NORM;

// definition macro: INFO, NAME, OP, plus optional FMT & VALIDATORS
template <typename INFO, typename NAME, typename OP
        , typename FMT = void, typename...Ts>
struct defn
{
    // look at `INFO` to pick appropriate `INFO_FN`
    static constexpr bool is_default   = std::is_void_v<typename OP::info_fn>;
    static constexpr bool is_single_sz = SINGLE_SZ<INFO>;
//    static constexpr bool is_cond      = INFO::value & SFX_CCODE_ALL::value;
    static constexpr auto fp_types     = ((1 << OP_SIZE_SINGLE)
                                         |(1 << OP_SIZE_XTND  )
                                         |(1 << OP_SIZE_PACKED)
                                         |(1 << OP_SIZE_DOUBLE)
                                         );
    static constexpr bool is_fp        = INFO::value & fp_types;

    // select INFO_FN from `INFO`. respect defined method
    using INFO_FN = std::conditional_t<
                          // if specified, use named `info_fn`
                          !is_default, typename OP::info_fn
    #if 0
                       , std::conditional_t<
                          // if conditional insn, use CCODE_NORM
                          is_cond, INFO_CCODE_NORM
                       , std::conditional_t<
                          // if floating point, use INFO_SIZE_FLT
                          is_fp, INFO_SIZE_FLT
    #endif
                       , std::conditional_t<
                          // if single width, use SIZE_VOID
                          is_single_sz, INFO_SIZE_VOID
                       ,
                          // default: INFO_SIZE_NORM
                          INFO_SIZE_NORM
                       >>;
   
    // forward to `tg_mcode_adder`
    using type = meta::list<INFO
                          , INFO_FN
                          , NAME
                          , typename OP::code
                          , typename OP::tst
                          , FMT             // formatter
                          , Ts...           // validators
                          >;
};

}


#endif

