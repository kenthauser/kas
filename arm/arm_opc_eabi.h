#ifndef KAS_ARM_ARM_OPC_EABI_H
#define KAS_ARM_ARM_OPC_EABI_H

// Support ARM EABI directives
// 
// Reference:
// https://github.com/ARM-software/abi-aa/releases/download/2021Q1/addenda32.pdf

// XXX the definition lists in this module should be transferred to `kbfd`
// XXX as they represent object file descriptions.


#include "target/tgt_directives_impl.h"
#include "kas_core/core_section.h"
#include "dwarf/dwarf_opc.h"
#include "kas/kas_string.h"
#include "dwarf/dwarf_emit.h"
#include <meta/meta.hpp>

namespace kas::arm::opc
{

namespace detail
{
    using namespace meta;
    // use a MACRO to define terms to cut down on clutter
    enum eabi_type { EABI_BYTE, EABI_NTSB, EABI_ULEB };

    #define EABI(N, V, T) list<KAS_STRING(#N), int_<V>, int_<EABI_ ## T>>

    using eabi_defns = meta::list<
        EABI(File               ,  1, BYTE)
      , EABI(Section            ,  2, BYTE)
      , EABI(Symbol             ,  3, BYTE)
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

    // the `arm` prefix should probably be added by `parser`
    #define EABI_ARCH(N, V) list<KAS_STRING("arm" #N), int_<V>>
    
    using eabi_arch_defns = meta::list<
        EABI_ARCH(pre-v4    , 0)
      , EABI_ARCH(v4        , 1)
      , EABI_ARCH(v4T       , 2)
      , EABI_ARCH(v5T       , 3)
      , EABI_ARCH(v5TE      , 4)
      , EABI_ARCH(v5TEJ     , 5)
      , EABI_ARCH(v6        , 6)
      , EABI_ARCH(v6KZ      , 7)
      , EABI_ARCH(v6T2      , 8)
      , EABI_ARCH(v6K       , 9)
      , EABI_ARCH(v7        , 10)
      , EABI_ARCH(v6-M      , 11)
      , EABI_ARCH(v6S-M     , 12)
      , EABI_ARCH(v7E-M     , 13)
      , EABI_ARCH(v8-A      , 14)
      , EABI_ARCH(v8-R      , 15)
      , EABI_ARCH(v8-M.baseline, 16)
      , EABI_ARCH(v8-M.mainline, 17)
      , EABI_ARCH(v8.1-A    , 18)
      , EABI_ARCH(v8.2-A    , 19)
      , EABI_ARCH(v8.3-A    , 20)
      , EABI_ARCH(v8.1-M.mainline, 21)
      >;

    using eabi_arch_profile = meta::list<
        list<KAS_STRING("")     , int_<0>>      // pre v7
      , list<KAS_STRING("A")    , int_<'A'>>    // "application"
      , list<KAS_STRING("R")    , int_<'R'>>    // "real-time"
      , list<KAS_STRING("M")    , int_<'M'>>    // "microcontroller"
      , list<KAS_STRING("S")    , int_<'S'>>    // "classic"
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

// generate `.ARM.attributes` after parse complete
struct gen_arm_attributes : core::core_section::deferred_ops
{
    gen_arm_attributes()
    {
        std::cout << "gen_arm_attributes::ctor" << std::endl;
        
        // see addenda32, section 3.2.1
        static const auto SHT_ARM_ATTRIBUTES = SHT_LOPROC + 3;
        auto& s = core::core_section::get(".ARM.attributes", SHT_ARM_ATTRIBUTES);
        s.set_deferred_ops(*this);
    }

    void gen_data(core::insn_inserter_t&& inserter) override;
};

struct arm_opc_eabi : parser::tgt_dir_opcode
{
    OPC_INDEX();
    const char *name() const override { return "EABI"; }

    struct attribute
    { 
        int       number;   // if name set, not a number
        std::string name;

        friend std::ostream& operator<<(std::ostream& os, attribute const& a)
        {
            if (a.name.size()) os << a.name; else os << a.number;
            return os;
        }
    };

    static auto& get_values()
    {
        static auto _values = new std::map<detail::eabi_tag_t const *, attribute>;
        return *_values;
    }

    // provide method to interpret args
    void tgt_proc_args(data_t& data, parser::tgt_dir_args&& args) const override
    {
        static gen_arm_attributes _;

        // access system tokens
        using expression::detail::tok_fixed;

        // create parser to xlate names to eabi defns
        // NB: also instantiates `eabi_tag_t` definitions
        static auto x3 = detail::eabi_tag_parser_t().x3();


        if (auto err = validate_min_max(args, 2, 2))
            return make_error(data, err);
        auto iter = args.begin();

        auto& tok_tag   = *iter++;
        auto& tok_value = *iter;

        unsigned  tag_value{};
        attribute value_value;
        
        if (auto p = tok_value.get_fixed_p())
            value_value.number = *p;
        else
            value_value.name   = tok_value.src();

        if (tok_tag.is_token_type(tok_fixed()))
        {
            tag_value = *tok_tag.get_fixed_p();
            auto p = detail::eabi_tag_t::lookup(tag_value);
            if (p)
            {
                std::cout << "EABI TAG: " << tag_value;
                std::cout << ": "  << p->name();
                std::cout << " = " << value_value << std::endl;
            }
            else
                std::cout << "EABI TAG: unknown" << std::endl;

            if (p)
                get_values().emplace(p, value_value);
        }
    }
};


void gen_arm_attributes::gen_data(core::insn_inserter_t&& inserter) 
{
    std::cout << "gen_arm_attributes::gen_data" << std::endl;

    auto& values = arm_opc_eabi::get_values();
    for (auto& obj : values)
    {
        std::cout << "EABI: " << obj.first->name();
        std::cout << " = " << obj.second << std::endl;
    }

    // emit `aeabi` section
    dwarf::emit_insn emit{inserter};

    // version header: version 'A'
    emit(dwarf::UBYTE(), 'A');

    // subsection header: length + zero-terminated vendor name
    auto& bgn = emit.get_dot();
    auto& end = core::core_addr_t::add();
    emit(dwarf::UWORD(), end - bgn);
    emit(dwarf::TEXT(), "aeabi");

    // declare attributes with `file` scope (along with length)
    auto& bgn_file = emit.get_dot();
    emit(dwarf::ULEB(), 1);         // file
    emit(dwarf::UWORD(), end - bgn_file);

    // all `kas` attributes are `file` scope
    for (auto& obj : values)
    {
        emit(dwarf::ULEB(), obj.first->value);

        switch (obj.first->coding)
        {
            case detail::EABI_BYTE:
                emit(dwarf::BYTE(), obj.second.number);
                break;
            case detail::EABI_ULEB:
                emit(dwarf::ULEB(), obj.second.number);
                break;
            case detail::EABI_NTSB:
                emit(dwarf::TEXT(), obj.second.name.c_str());
                break;
            default:
                throw std::logic_error{"EABI: invalid coding"};
        }
    }
    
    emit(end);      // drop the `end` address label
}
    
}
#endif
