#ifndef KAS_BSD_PSEUDO_OPS_DEF_H
#define KAS_BSD_PSEUDO_OPS_DEF_H

#include "bsd_stmt.h"

#include "bsd_ops_core.h"
#include "bsd_ops_symbol.h"
#include "bsd_ops_section.h"
#include "bsd_ops_dwarf.h"
#include "bsd_ops_cfi.h"

#include "parser/parser.h"
#include "parser/sym_parser.h"

#include "kas/defn_utils.h"
#include "kas/kas_string.h"
#include "utility/string_mpl.h"

#include "expr/format_ieee754.h"

#include "dwarf/dwarf_frame_data.h"


namespace kas::bsd::parser::detail
{
// NB: in namespace detail, include meta
using namespace meta;
using namespace kas::core::opc;


// declare constants for use as BSD fixed arguments
//
// Some BSD pseudo-op names are really special cases
// of generic operations. Examples are
//      `.even` is exactly `.align 1`
// and  `.text` is exactly `.section ".text"`
//
// To efficiently handle these "BSD aliases", fixed arguments
// may also be passed to the BSD directive "op-codes" in addition
// to parsed arguments.
//
// To facilitate display of the fixed arguments in the test
// fixture traces as well as actual evaluation, two "constant"
// types are defined. These types have two characteristics:
//
//  method `operator const char *()` for test fixture &
//  static member `value` for evaluation

// NB: `std::integral_constant` has both  `value` & `operator T()`
template <typename STR>
struct k_string : STR
{
    //static constexpr const char *value = STR{};
    static constexpr short n_value = 0;
};

// create a "fixed constant" type which has:
//    -- derives from kas_string for test fixure messages
//    -- an `operator const T()` for retrieving constant value

template <typename T, T N>
struct k_constant : i2s<N>
{
    static constexpr T n_value = N;
   // constexpr operator T () const { return N; }
};

#define STR KAS_STRING 

//
// Define dwarf frame directives
//

// Canonical frame address
// Create a MACRO to prepend "cfi_", et al., thus easing name creation

#define CFI(s)      string::str_cat<STR("cfi_"), s>

template <typename NAME, typename VALUE>
struct gen_dwarf_cmd
{
    using name  = CFI(NAME);
    using type  = list<name, bsd_cfi_cmd, k_constant<short, VALUE::value>>;
};

struct gen_dwarf_op_cmds
{
    using dw_cmd_types = dwarf::df_cmd_types;
#if 0
    using names        = at_c<zip<dw_cmd_types>, 0>;
    using indexes      = std::make_index_sequence<names.size()>;
    using type         = transform<names, indexes, quote_trait<gen_dwarf_cmd>>;
#else
    template <typename CMD>
    struct op2cmd
    {
        using index = find_index<dw_cmd_types, CMD>;
        using type = _t<gen_dwarf_cmd<at_c<CMD, 0>, index>>;
    };

    using type = transform<dw_cmd_types, quote_trait<op2cmd>>;
#endif
};

// pick up dwarf frame commands from dwarf definitions
using dwarf_op_cmds  = _t<gen_dwarf_op_cmds>;

// additional BSD pseudo-ops for frame generation
using dwarf_bsd_cmds = list<
    // bsd pseudo-op to generate call frame
      list<CFI(STR("sections")),   bsd_cfi_sections>
#if 0
// GNU Extensions
    , X_DEFN_CFI<CFI("personality")>
    , X_DEFN_CFI<CFI("personality_id")>
    , X_DEFN_CFI<CFI("lsda")>
    , X_DEFN_CFI<CFI("inline_lsda")>
// BSD (and GNU) Aliases
    , gen_dwarf_cmd<STR("def_cfa_factored"), dwarf::DW_def_cfa_sf>
#endif
    >;

#undef CFI

//
//
// 

using _ZERO         = k_constant<short, 0>;
using _ONE          = k_constant<short, 1>;
using _TWO          = k_constant<short, 2>;

using _STB_LOCAL    = k_constant<short, STB_LOCAL>;
using _STB_GLOBAL   = k_constant<short, STB_GLOBAL>;

using _SEG_TEXT     = STR(".text");
using _SEG_DATA     = STR(".data");
using _SEG_BSS      = STR(".bss");

//
// space separated directives: name, bsd_opcode, [<additional args>]
//

template<> struct space_ops_v<bsd_basic_tag> : list<
  list<STR("loc"),          bsd_loc>
, list<STR("file"),         bsd_file>

// NB .type can be space separated in GNU STT_ variant...
, list<STR("type"),         bsd_elf_type>

> {};

//
// comma separated directives: name, bsd_opcode, [<additional args>]
//

template<> struct comma_ops_v<bsd_dwarf_tag> : //dwarf_op_cmds{};
        concat<dwarf_op_cmds, dwarf_bsd_cmds> {};

using bsd_fixed_cmds = list<
// fixed data ops independent of architecture
  list<STR("byte"),         opc_fixed<std::int8_t>>
, list<STR("2byte"),        opc_fixed<std::int16_t>>
, list<STR("4byte"),        opc_fixed<std::int32_t>>
, list<STR("8byte"),        opc_fixed<std::int64_t>>

#define PSEUDO_WORD_16BITS
#ifdef  PSEUDO_WORD_16BITS
, list<STR("word"),         opc_fixed<std::int16_t>>
, list<STR("long"),         opc_fixed<std::int32_t>>
#else
, list<STR("word"),         opc_fixed<std::int32_t>>
, list<STR("long"),         opc_fixed<std::int64_t>>
#endif
, list<STR("hword"),        opc_fixed<std::int16_t>>
, list<STR("short"),        opc_fixed<std::int16_t>>

, list<STR("quad"),         opc_fixed<std::int64_t>>
, list<STR("sleb128"),      opc_sleb128>
, list<STR("uleb128"),      opc_uleb128>

, list<STR("ascii"),        opc_string<std::false_type>>
, list<STR("asciz"),        opc_string<std::true_type>>
, list<STR("string"),       opc_string<std::true_type>>

, list<STR("string8"),      opc_string<std::true_type, std::uint8_t>>
, list<STR("string16"),     opc_string<std::true_type, std::uint16_t>>
, list<STR("string32"),     opc_string<std::true_type, std::uint32_t>>
, list<STR("string64"),     opc_string<std::true_type, std::uint64_t>>
>;


template <typename ARG>
struct combine_fixed
{
    using type = list<at_c<ARG, 0>, bsd_fixed<at_c<ARG, 1>>>;
};

using X_bsd_fixed = transform<bsd_fixed_cmds, quote_trait<combine_fixed>>;

template <> struct comma_ops_v<bsd_fixed_tag> : X_bsd_fixed {};

#if 0
template<> struct comma_ops_v<bsd_float_tag> : list<
// fixed data ops independent of architecture
  list<STR("single"),       opc_float<expression::ieee754::FMT_IEEE_32_SINGLE>>
//, list<STR("double"),       bsd_fixed<opc_float<m68k::m68k_format_float::FMT_IEEE_64_DOUBLE>>>
//, list<STR("extended"),     bsd_fixed<opc_float<m68k::m68k_format_float::FMT_M68K_80_EXTEND>>>
> {};
#endif

template<> struct comma_ops_v<bsd_basic_tag> : list<
// section ops
  list<STR("section"),      bsd_section>
, list<STR("pushsection"),  bsd_push_section>
, list<STR("popsection"),   bsd_pop_section>
, list<STR("previous"),     bsd_previous_section>

, list<STR("text"),         bsd_section, _SEG_TEXT>
, list<STR("data"),         bsd_section, _SEG_DATA>
, list<STR("data1"),        bsd_section, _SEG_DATA, _ONE>
, list<STR("data2"),        bsd_section, _SEG_DATA, _TWO>
, list<STR("bss"),          bsd_section, _SEG_BSS>

// program counter ops
, list<STR("skip"),         bsd_skip>
, list<STR("org"),          bsd_org>
, list<STR("align"),        bsd_align>
, list<STR("even"),         bsd_align, _ONE>

// symbol ops
, list<STR("local"),        bsd_sym_binding, _STB_LOCAL>
, list<STR("globl"),        bsd_sym_binding, _STB_GLOBAL>
, list<STR("global"),       bsd_sym_binding, _STB_GLOBAL>

, list<STR("comm"),         bsd_common, _STB_GLOBAL>
, list<STR("lcomm"),        bsd_common, _STB_LOCAL>

// ELF ops
, list<STR("size"),         bsd_elf_size>
, list<STR("ident"),        bsd_elf_ident>

> {};

#undef STR
}
#endif
