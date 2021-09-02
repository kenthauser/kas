#ifndef KAS_ARM_ARM_OPC_EABI_H
#define KAS_ARM_ARM_OPC_EABI_H

// Support ARM EABI directives
// 
// Reference:
// https://github.com/ARM-software/abi-aa/releases/download/2021Q1/addenda32.pdf


#include "target/tgt_directives_impl.h"
#include "kas/kas_string.h"
#include <meta/meta.hpp>

namespace kas::arm::opc
{

namespace detail
{
    using namespace meta;
    // use a MACRO to define terms to cut down on clutter
    enum eabi_type { EABI_INT, EABI_NTSB, EABI_ULEB };

    #define EABI(N, V, T) list<KAS_STRING(#N), int_<V>, int_<EABI_ ## T>>

    using eabi_defns = meta::list<
        EABI(File               ,  1, INT)
      , EABI(Section            ,  2, INT)
      , EABI(Symbol             ,  3, INT)
      , EABI(CPU_raw_name       ,  4, NTSB)
      , EABI(CPU_name           ,  5, NTSB)
      , EABI(CPU_arch           ,  6, ULEB)
      , EABI(CPU_arch_profile   ,  7, ULEB)
      , EABI(ARM_ISA_use        ,  8, ULEB)
      , EABI(THUMB_ISA_use      ,  9, ULEB)
      , EABI(FP_arch            , 10, ULEB)
      , EABI(VFP_arch           , 10, ULEB)     // former TAG name

      , EABI(WMMX_arch          , 11, ULEB)
      , EABI(Advanced_SIMD_arch , 12, ULEB)
      , EABI(PCS_config_arch    , 13, ULEB)
      , EABI(ABI_PCS_R9_use     , 14, ULEB)
      , EABI(ABI_PCS_RW_data    , 15, ULEB)
      , EABI(ABI_PCS_RO_data    , 16, ULEB)
      , EABI(ABI_PCS_GOT_use    , 17, ULEB)
      , EABI(ABI_PCS_wchar_t    , 18, ULEB)
      , EABI(ABI_FP_rounding    , 19, ULEB)
      , EABI(ABI_FP_denormal    , 20, ULEB)
      
      , EABI(ABI_FP_exceptions  , 21, ULEB)
      , EABI(ABI_FP_user_exceptions, 22, ULEB)
      , EABI(ABI_FP_number_model, 23, ULEB)
      , EABI(ABI_align_needed   , 24, ULEB)
      , EABI(ABI_align8_needed  , 24, ULEB)    // former TAG name
      , EABI(ABI_align_preserved, 25, ULEB)
      , EABI(ABI_align8_preserved, 25, ULEB)    // former TAG name
      , EABI(ABI_enum_size      , 26, ULEB)
      , EABI(ABI_HardFP_use     , 27, ULEB)
      , EABI(ABI_VFP_args       , 28, ULEB)
      , EABI(ABI_WMMX_args      , 29, ULEB)
      , EABI(ABI_optimization_goals, 30, ULEB)
      
      , EABI(ABI_FP_optimization_goals, 31, ULEB)
      , EABI(compatibility      , 32, NTSB)
      , EABI(CPU_unaligned_access, 34, ULEB)
      , EABI(FP_HP_extension    , 36, ULEB)         // former TAG name
      , EABI(FP_VHP_extension   , 36, ULEB)
      , EABI(ABI_FP_16bit_format, 38, ULEB)
      
      , EABI(MPextension_use    , 42, ULEB)
      , EABI(DIV_use            , 44, ULEB)
      , EABI(DIV_extension      , 46, ULEB)
      , EABI(MVE_arch           , 48, ULEB)
      , EABI(PAC_extension      , 50, ULEB)
      , EABI(BTI_extension      , 52, ULEB)
      , EABI(nodefaults         , 64, ULEB)
      
      , EABI(also_compatible_with, 65, ULEB)
      , EABI(conformance        , 67, ULEB)
      , EABI(T2EE_use           , 66, ULEB)
      , EABI(Virtualization_use , 68, ULEB)
      , EABI(MPextension_use_ALT, 70, ULEB)     // recoded to 42
      , EABI(FramePointer_use   , 72, ULEB)
      , EABI(BTI_use            , 74, ULEB)
      , EABI(PACRET_use         , 76, ULEB)
      >;

    // forward declare adder
    struct eabi_tag_adder;

    // type to store EABI definitions
    struct eabi_tag_t
    {
        using ADDER  = eabi_tag_adder;

        using NAME_LIST = list<int_<0>>;

        using XLATE_LIST = list<list<const char *, NAME_LIST>>;

        // constructor called from default `adder`
        template <typename NAME, typename N, typename V, typename T>
        constexpr eabi_tag_t(list<list<NAME>, list<N, V, T>>)
            : name_idx  { NAME::value + 1 }
            , value     { V::value        }
            , coding    { T::value        }
            {}

        // initialized by default `adder`
        static inline const char       * const *names_base;
        static inline const eabi_tag_t  *  defns_base;
        static inline unsigned           defns_cnt;

        template <typename DEFNS>
        static void init(const eabi_tag_t *const defns, unsigned cnt, DEFNS)
        {
            defns_base = defns;
            defns_cnt  = cnt;
            names_base = at_c<DEFNS, 0>::value;
        }

        const char *name() const
        {
            if (name_idx)
                return names_base[name_idx - 1];
            return {};
        }
        
        // find DEFN by value
        static eabi_tag_t const *lookup(unsigned n)
        {
            auto cnt = defns_cnt;
            for (auto p = defns_base; cnt; ++p, --cnt)
                if (p->value == n)
                    return p;
            return {};
        }

        uint8_t name_idx;
        uint8_t value;
        uint8_t coding;
    };

    // ADDER adds instances to directives parser
    struct eabi_tag_adder
    {
        // parser return type
        using defn_t     = eabi_tag_t;
        using VALUE_T    = eabi_tag_t const *;
        using XLATE_LIST = typename eabi_tag_t::XLATE_LIST;

        template <typename PARSER>
        eabi_tag_adder(PARSER) : defns(PARSER::sym_defns)
        {
            // expose defns from sym_parser
            using all_types_defns = typename PARSER::all_types_defns; 

            // pass init definitions back to calling type
            defn_t::init(defns, PARSER::sym_defns_cnt, all_types_defns());
        }

        template <typename X3>
        void operator()(X3& x3, unsigned count) const
        {
            auto& add = x3.add;
            for (auto p = defns; count--; ++p)
                if (auto name = p->name())
                    add(name, p);
        }

        defn_t const *const defns;
    };
    
    using eabi_tag_parser_t = parser::sym_parser_t<eabi_tag_t, eabi_defns>;
}

struct arm_opc_eabi : parser::tgt_dir_opcode
{
    OPC_INDEX();
    const char *name() const override { return "EABI"; }

    // provide method to interpret args
    void tgt_proc_args(data_t& data, parser::tgt_dir_args&& args) const override
    {
        // access system tokens
        using expression::detail::tok_fixed;

        // create parser to xlate names to eabi defns
        // NB: also instantiates `eabi_tag_t` definitions
        static auto x3 = detail::eabi_tag_parser_t().x3();


        if (auto err = validate_min_max(args, 2, 2))
            return make_error(data, err);
#if 0
        auto iter = args.begin();

        auto sym_p = iter->template get_p<core::symbol_ref>();
        if (!sym_p)
            return make_error(data, "symbol required", *iter);

        // several formats for symbol type:
        // eg: the following are equiv: @function, @2, STT_FUNC
        auto& loc = *++iter;
        auto value = -1;
        if (iter->is_token_type(tok_bsd_at_ident()))
            value = parser::get_symbol_type(*iter, true);
        else if (iter->is_token_type(tok_bsd_at_num()))
            value = *iter->get_fixed_p();
        else if (iter->is_token_type(tok_bsd_ident()))
            value = parser::get_symbol_type(*iter, false);

        if (value < 0)
            return make_error(data, "invalid symbol type", *iter);

        base_op.proc_args(data, value, sym_p->get(), loc);
#else
        auto iter = args.begin();

        auto& tok_tag   = *iter++;
        auto& tok_value = *iter;

        unsigned tag_value{};
        unsigned value_value{};
        
        if (auto p = tok_value.get_fixed_p())
            value_value = *p;

        if (tok_tag.is_token_type(tok_fixed()))
        {
            tag_value = *tok_tag.get_fixed_p();
#if 1
            auto p = detail::eabi_tag_t::lookup(tag_value);
            if (p)
            {
                std::cout << "EABI TAG: " << tag_value;
                std::cout << ": " << p->name()  << " = " << value_value << std::endl;
            }
            else
                std::cout << "EABI TAG: unknown" << std::endl;
#endif
        }
#endif
    }


    
    
};
    
}
#endif
