#ifndef KAS_BSD_PSEUDO_OPS_DEF_H
#define KAS_BSD_PSEUDO_OPS_DEF_H

#include "bsd_stmt.h"

#include "bsd_ops_core.h"
#include "bsd_ops_symbol.h"
#include "bsd_ops_section.h"
#include "bsd_ops_dwarf.h"
#include "bsd_ops_cfi.h"

#include "parser/sym_parser.h"

#include "kas/defn_utils.h"
#include "kas/kas_string.h"
#include "utility/string_mpl.h"

namespace kas::bsd
{
namespace detail
{
    // NB: in namespace detail, include meta
    using namespace meta;
    using namespace kas::core::opc;


    // declare constants for use as BSD fixed arguments
    //
    // Some BSD pseudo-op names are really special cases
    // of generic operations. Examples are
    //      `.even` is exactly `.align 1`
    // and  `.text` is exactly `.segment ".text"`
    //
    // To efficiently handle these "BSD aliases", fixed arguments
    // may also be passed to the BSD "op-codes" in addition to
    // parsed arguments.
    //
    // To facilitate display of the fixed arguments in the test
    // fixture traces as well as actual evaluation, two "constant"
    // types are defined. These types have two characteristics:
    //
    //  method `operator const char *()` for test fixture &
    //  static member `value` for evaluation

    // `integral_constant` has both  `value` & `operator T()`
    template <typename STR>
    struct k_string : STR
    {
        static constexpr const char *value = STR{}; 
    };

    // create a "fixed constant" type which has:
    //    -- a `value` member holding type's value
    //    -- an `operator const char *()` for initing test fixture messages
    template<typename T, T N>
    struct k_constant : std::integral_constant<T, N>
    {
        constexpr operator const char *() const
        {
            return i2s<N>();
        }
    };

#define STR KAS_STRING 

    using _ZERO    = k_constant<short, 0>;
    using _ONE     = k_constant<short, 1>;
    using _TWO     = k_constant<short, 2>;

    using _STB_LOCAL   = k_constant<short, STB_LOCAL>;
    using _STB_GLOBAL  = k_constant<short, STB_GLOBAL>;

    using _SEG_TEXT = k_string<STR(".text")>;
    using _SEG_DATA = k_string<STR(".data")>;
    using _SEG_BSS  = k_string<STR(".bss")>;

    // Canonical frame address
    // NB: Some of the CFI names are so long they exceeded 16 character macro limit
    // create a MACRO to prepend "cfi_", thus easing name creation
#define CFI(s)      ::kas::string::str_cat<STR("cfi_"), STR((s))>
#define CFI_CFA(s)  ::kas::string::str_cat<STR("cfi_def_cfa_"), STR((s))>
#define CFI_ADJ(s)  ::kas::string::str_cat<STR("cfi_adjust_"), STR((s))>

template <typename CMD>
using X_DEFN_CFI = list<CMD, bsd_cfi_undef, CMD>;

template <typename CMD, int DW_CMD, int num_args>
using DEFN_CFI = list<CMD, bsd_cfi_oper, k_constant<short, DW_CMD>, k_constant<short, num_args>>;



    // pseudo ops defns: name, core_opcode, [<additional args>]
    template<> struct comma_ops_v<bsd_basic_tag> : list<
    // fixed data ops
          list<STR("byte"),         bsd_fixed<opc_fixed<std::int8_t>>>
        , list<STR("word"),         bsd_fixed<opc_fixed<std::int16_t>>>
        , list<STR("long"),         bsd_fixed<opc_fixed<std::int32_t>>>
        , list<STR("quad"),         bsd_fixed<opc_fixed<std::int64_t>>>
        , list<STR("sleb128"),      bsd_fixed<opc_sleb128>>
        , list<STR("uleb128"),      bsd_fixed<opc_uleb128>>
        , list<STR("ascii"),        bsd_fixed<opc_string<std::false_type>>>
        , list<STR("asciz"),        bsd_fixed<opc_string<std::true_type>>>
        , list<STR("string"),       bsd_fixed<opc_string<std::true_type>>>
//        , list<STR("single"),       bsd_fixed<opc_float<m68k::m68k_format_float::FMT_IEEE_32_SINGLE>>>
//        , list<STR("double"),       bsd_fixed<opc_float<m68k::m68k_format_float::FMT_IEEE_64_DOUBLE>>>
//        , list<STR("extended"),     bsd_fixed<opc_float<m68k::m68k_format_float::FMT_M68K_80_EXTEND>>>
#if 0
        //, list<STR("string8"),      opc_string<std::true_type>, std::uint8_t>
        //, list<STR("string16"),     opc_string<std::true_type>, std::uint16_t>
        //, list<STR("string32"),     opc_string<std::true_type>, std::uint32_t>
        //, list<STR("string64"),     opc_string<std::true_type>, std::uint64_t>
#endif

    // program counter ops
        , list<STR("skip"),         bsd_skip>
        , list<STR("org"),          bsd_org>
        , list<STR("align"),        bsd_align>
#if 1
        , list<STR("even"),         bsd_align, _ONE>
    // section ops
        , list<STR("section"),      bsd_section>
        , list<STR("pushsection"),  bsd_push_section>
        , list<STR("popsection"),   bsd_pop_section>
        , list<STR("previous"),     bsd_previous_section>

        , list<STR("text"),         bsd_section, _SEG_TEXT>
        , list<STR("data"),         bsd_section, _SEG_DATA>
        , list<STR("data1"),        bsd_section, _SEG_DATA, _ONE>
        , list<STR("data2"),        bsd_section, _SEG_DATA, _TWO>
        , list<STR("bss"),          bsd_section, _SEG_BSS>
    // symbol ops
        , list<STR("globl"),        bsd_sym_binding, _STB_GLOBAL>
        , list<STR("local"),        bsd_sym_binding, _STB_LOCAL>

        , list<STR("comm"),         bsd_common, _STB_GLOBAL>
        , list<STR("lcomm"),        bsd_common, _STB_LOCAL>

    // ELF ops
        , list<STR("size"),         bsd_elf_size>
        , list<STR("ident"),        bsd_elf_ident>

    // I have no idea where this list of insns comes from. I copied from `gas::dw2gencfi.c`
        , list<CFI("sections"),     	    bsd_cfi_sections>
        , list<CFI("startproc"),            bsd_cfi_startproc>
        , list<CFI("endproc"),              bsd_cfi_endproc>

        , DEFN_CFI<CFI("offset"), dwarf::DF_offset, 2> 
        , DEFN_CFI<CFI("def_cfa"), dwarf::DF_def_cfa, 2>

        , X_DEFN_CFI<CFI("fde_data")>
        //, X_DEFN_CFI<CFI("def_cfa")>
        , X_DEFN_CFI<CFI_CFA("register")>
        , X_DEFN_CFI<CFI_CFA("offset")>
        //, X_DEFN_CFI<CFI("offset")>
        , X_DEFN_CFI<CFI("personality")>
        , X_DEFN_CFI<CFI("personality_id")>
        , X_DEFN_CFI<CFI("lsda")>
        , X_DEFN_CFI<CFI("inline_lsda")>
        , X_DEFN_CFI<CFI_ADJ("cfa_offset")>
#undef CFI
#undef CFI_CFA
#undef CFI_ADJ

#endif
        > {};

    // space separated opcodes
    template<> struct space_ops_v<bsd_basic_tag> : list<
          list<STR("loc"),          bsd_loc>
        , list<STR("file"),         bsd_file>
        
        // NB .type can be space separated in GNU STT_ variant...
        , list<STR("type"),         bsd_elf_type>
    
        > {};
#undef STR
    
    // support routines for `pseudo_op_t`
    static constexpr short to_short(const char *) { return 0; }
    template <short N>
    static constexpr short to_short(std::integral_constant<short, N>) { return N; }
                                    
    template <typename OP>
    static auto& get_opcode()
    {
        static OP opcode;
        return opcode;
    }
    
    template <typename OP>
    static opcode& get_ref()
    {
        return get_opcode<OP>();
    }

    // type for pseudo-op definitions
    struct pseudo_op_t
    {
        static constexpr auto MAX_FIXED_ARGS = 2;

        template <typename NAME, typename OPCODE, typename...ARGS>
        constexpr pseudo_op_t(list<NAME, OPCODE, ARGS...>)
            : name      { NAME::value         }
            , arg_c     { sizeof...(ARGS)     }
            , str_v     { ARGS()...           }
            , num_v     { to_short(ARGS())... }
            , op        { get_ref<OPCODE>     }
            , proc_args { &pseudo_op_t::do_proc<OPCODE> } 
            {}

        template <typename OP, typename...Ts>
        opcode& do_proc(opcode::data_t& data, bsd_args&& args) const
        {
            auto& op = get_opcode<OP>();
            op.proc_args(data, std::move(args), arg_c, str_v, num_v);
            return op;
        }

        const char *name;
        opcode&   (*op)();
        opcode&   (pseudo_op_t::*proc_args)(opcode::data_t&, bsd_args&&) const;
        const char *str_v[MAX_FIXED_ARGS];
        short       num_v[MAX_FIXED_ARGS];
        short       arg_c;
    };

    using comma_defs = all_defns<comma_ops_v, bsd_pseudo_tags>;
    using space_defs = all_defns<space_ops_v, bsd_pseudo_tags>;

    static const auto comma_ops = parser::sym_parser_t<pseudo_op_t, comma_defs>();
    static const auto space_ops = parser::sym_parser_t<pseudo_op_t, space_defs>();
}

auto const comma_op_x3 = x3::no_case[x3::lexeme['.' >> detail::comma_ops.x3()]];
auto const space_op_x3 = x3::no_case[x3::lexeme['.' >> detail::space_ops.x3()]];

//
// implement bsd_stmt methods for pseudo-ops
//

std::string bsd_stmt_pseudo::name() const
{
    return std::string("BSD_PSEUDO:") + op->name;
}

opcode *bsd_stmt_pseudo::gen_insn(opcode::data_t& data)
{
    return &(op->*op->proc_args)(data, std::move(v_args));
}

void bsd_stmt_pseudo::print_args(kas::parser::print_obj const& fn) const
{
    // if fixed args, copy to container
    auto n = op->arg_c;
    std::vector<const char *> xtra_args(op->str_v, op->str_v + n);
    
    if (n)
        fn(xtra_args, v_args);
    else
        fn(v_args);
}
}
#endif
