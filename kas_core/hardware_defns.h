#ifndef KAS_KAS_CORE_HARDWARE_DEFNS_H
#define KAS_KAS_CORE_HARDWARE_DEFNS_H

//
// Define methods to analyze CPU definitions
//
// Basic defintions held for each CPU are:
//
//  - is_bits.  Test if `CPU` subclass of required CPU
//  - has_bits. Test if `feature` is set for defined CPU
//  - cp_bits.  Test if `feature` is set for defined {co-processor, ...}

// Design of `insn` format is to allow an insn to specify one `CPU` or `FEATURE`
// which must be present for the `insn` to be valid. As `insn` defns are type lists
// and any `feature` or `cpu` may be specified. `void` always matches.

// Required `insn` defn to also be able to yield defn type. (testable)
// In particular need `insn` defn ::name, && ::type.

// Tests used in common code:
//
// NB: All defns return TST::name if false. nullptr if true.
// Defns come in `is` & `has` to test CPU & Feature.
// Corresponding `is_not` & `has_not` reverse test.

// defns.is(CPU{})
// defns.is_not(CPU{})
// defns.has(FEATURE{})
// defns.has_not(FEATURE{})
// defns[TST{}]     // automatically test CPU or FEATURE
// defns[hw_tst]    // test using `hw_tst` instance (see below).
// NB: test `hw_void` is a stand-in for `void` in above tests
// (can't instantiate void) which take a type.

//
// TST subtype used to store `insn` tests. Resolves `cpu`, `feature` & `cp_feature`
// as two unsigned bytes. 
//
// struct hw_tst { uint8_t cpu, uint8_t feature; };
//
// hw_tst has the following coding:
//
// if `feature` == 0 && `cpu` == 0 -> void. Always true.
// if `feature` == 0 && `cpu` != 0 -> cpu_defn.is(cpu-1).
// if `feature` != 0 && cpu == 0 --> cpu_defn.bitset(feature - 1);
// if `feature` != 0 && cpu != 0 --> (test CP feature && CP defn)
//      return cpu_defn(cpu-1) && cp_defn(feature - 1)


#include "hardware_features.h"      // need: hw_bitset_t
#include "utility/align_as_t.h"

#include <type_traits>
#include <array>
#include <bitset>
#include <meta/meta.hpp>

namespace kas::core::hardware
{

// tag for test that matches all
//struct hw_void;// {};


namespace detail
{
    using namespace meta;
    using NAME_T = const char *;


    template <typename T, typename NAME = void>
    struct hw_name
    {
        static constexpr NAME_T value = NAME::value;
    };

    template <>
    struct hw_name<void, void>
    {
        static constexpr NAME_T value = "void";
    };

#if 0
    template <>
    struct hw_name<hw_void, void>
        : hw_name<void> {};
#endif
        
    template <typename T>
    struct hw_name<T, std::void_t<typename T::name>>
            : hw_name<void, typename T::name> {};

#if 0
    template <typename T>
    struct hw_name<T, void>
    {
        static_assert(std::is_void<T>(), "No name for type");
    }
#endif
    
    // create `char **list` for `is_names`
    template <typename HW_LIST> struct is_name_list;

    template <typename...Args>
    struct is_name_list<list<Args...>>
    {
        // zero array not allowed
        static constexpr auto _size = std::max<unsigned>(1, sizeof...(Args));
        static constexpr NAME_T value[_size] = { hw_name<Args>::value... };
    };

    // create `char ***list` for `has_names`
    template <typename HW_LIST_LIST> struct has_name_list;

    template <typename...Args>
    struct has_name_list<list<Args...>>
    {
        // zero array not allowed
        static constexpr auto _size = std::max<unsigned>(1, sizeof...(Args));
        static constexpr NAME_T const *value[_size] = { is_name_list<Args>::value... };
    };

    // calculate `isa_bitset` for `IS_TYPE` for each type in `IS_LIST`
    template <typename IS_LIST, typename IS_TYPE, typename REST = IS_LIST>
    constexpr auto hw_isa_bitset(hw_bitset_t<IS_LIST> N = 0)
    {
        if constexpr (REST::size() == 0) {
            return N;
        } else {
            auto is_a = std::is_base_of<front<REST>, IS_TYPE>();
            N |= is_a << (IS_LIST::size() - REST::size());
            return hw_isa_bitset<IS_LIST, IS_TYPE, drop_c<REST, 1>>(N);
        }
    }

    template <typename T, typename...Ts>
    constexpr auto init_list(meta::list<Ts...>)
    {
        return std::array<T, sizeof...(Ts)>{ Ts()... };
    }

}

// wrapper for co-processor KEY & partial selection template
template <typename KEY, template <typename> class CP>
struct hw_cp_def
{
    using key = KEY;
    using ftl = typename CP<void>::type::ftl;

    template <typename T>
    using invoke = meta::_t<CP<T>>;
};

template <typename IS, typename...CP_DEFNS>
struct hw_defs
{
    // declare member types & `static constexpr` variables
    using is_dflt  = meta::front<IS>;
    using is_list  = meta::pop_front<IS>;

    using is_has   = typename is_dflt::ftl;
    using cp_keys  = meta::list<typename CP_DEFNS::key ...>;
    using has_list = meta::list<is_has, typename CP_DEFNS::ftl ...>;

    //
    // identifier for test. Format in file header comment
    //
    struct hw_tst// : kas::detail::alignas_t<hw_tst, uint16_t>
    {
       // using base_t = kas::detail::alignas_t<hw_tst, uint16_t>;
        // using base_t::base_t;       // ctor for ints. not constexpr.
        
        // public ctors
        // NB: `void` not allowed as ctor argument
        // overloads to construct default & `hw_void`
        //constexpr hw_tst() : value{} {}
        constexpr hw_tst(uint16_t value = {}) : value{value}   {}
        //constexpr hw_tst(hw_void) : hw_tst()  {}

        template <typename T, T N>
        constexpr hw_tst(std::integral_constant<T, N>) : value{N} {}

        // ctor: look through all type lists & delegate to sort it out.
        template <typename T, typename = std::enable_if_t<!std::is_integral_v<T>>>
        constexpr hw_tst(T) : hw_tst(T()
                                    , meta::find_index<is_list, T>()
                                    , meta::find_index<is_has,  T>()
                                    , meta::find_index<typename CP_DEFNS::ftl, T>()...)
                            {}
    private:
        // actual ctors...
        // ctor for CPU
        template <typename T, typename CPU, typename...Ts,
                  typename = std::enable_if_t<!std::is_same_v<CPU, meta::npos>>>
        constexpr hw_tst(T, CPU, Ts...)
                : value { to_value(CPU()+1) }
                {}
        
        // ctor for FEATURE
        template <typename T, typename FEATURE, typename...Ts,
                  typename = std::enable_if_t<!std::is_same_v<FEATURE, meta::npos>>>
        constexpr hw_tst(T, meta::npos, FEATURE, Ts...args)
                : value { to_value(sizeof...(CP_DEFNS) - sizeof...(Ts), FEATURE()+1) }
                {}

        // ctor to recurse on features
        template <typename T, typename...Ts>
        constexpr hw_tst(T, meta::npos, meta::npos, Ts...args)
                : hw_tst(T(), meta::npos(), args...)
                {}
        
        // ctor fail on no match
        template <typename T, typename...Ts>
        constexpr hw_tst(T, meta::npos, Ts...args) : hw_tst()
                {
                    //static_assert(std::is_void_v<T>, "invalid type for hw_tst");
                }
                
    public:
        constexpr bool operator==(hw_tst const& other) const
        {
            return value == other.value;
        }
        
        constexpr bool operator!=(hw_tst const& other) const
        {
            return !(*this == other);
        }

        // access value
        constexpr uint8_t cpu()       const { return value; }
        constexpr uint8_t feature()   const { return value >> 8; }
        constexpr operator uint16_t() const { return value; }

        const char *name() const;
        bool is_cp(hw_tst const& key) const;
        
    //private:
        static constexpr uint16_t to_value(uint8_t cpu, uint8_t feature = 0)
        {
            return cpu | (feature << 8);
        }

        friend hw_defs;
        uint16_t value;
    };

    // hw_void: match everything
    struct hw_void : std::integral_constant<int16_t, 0> {};



    // create arrays of names
    static constexpr auto is_names    = detail::is_name_list<is_list>::value;
    static constexpr auto has_names   = detail::has_name_list<has_list>::value;

    // use a single instantitation of std::bitset<>
    // calculate size of largest `has` list & `is` list
    using has_sizes     = meta::_t<meta::transform<has_list, meta::quote<meta::size>>>;
    using max_has_size  = meta::_t<meta::apply<meta::quote<meta::max>, has_sizes>>;
    using bitset_size_t = meta::max<meta::size<is_list>, max_has_size>;
    using bitset_t      = std::bitset<bitset_size_t::value>;
    using has_bitset_t  = std::array<bitset_t, has_list::size()>;

    // declare `instance` variables
    // NB: `has_bits` must be a `tuple` for initialization
    detail::NAME_T current_name;
    bitset_t       is_bits;
    has_bitset_t   has_bits;

    // default constructor
    constexpr hw_defs() 
    {
        set(is_dflt{});
    }

    template <typename T>
    hw_defs(T)
    {
        set(T{});
    }

public:
    // index `hw_defs` using feature enum
    constexpr const char *operator[] (hw_tst tst) const
    {
        if (tst.feature() == 0) {
            // testing CPU or nothing
            if (tst.cpu() == 0 || is_bits[tst.cpu()-1])
                return nullptr;
            return is_names[tst.cpu()-1];
        } 
        
        // if `co-processor` feature, test key
        if (tst.cpu())
            if (auto msg = cp_tst(tst.cpu()))
                return msg;

        // testing a feature
        if (has_bits[tst.cpu()][tst.feature()-1])
            return nullptr;
        
        return has_names[tst.cpu()][tst.feature() - 1];
    }

    const char *cp_tst(uint8_t cp_id) const
    {
        // I can't figure out how to make this constexpr 2018/04/30 Kent
        static hw_tst tst_cp_keys[] = { typename CP_DEFNS::key()... };
        return (*this)[tst_cp_keys[cp_id-1]];
    }
    
    struct hw_init
    {
        // define the tuple to hold `has_bits`
        using has_init_t = meta::transform<has_list, meta::quote<detail::hw_bitset_t>>;
        //using has_bits_t = meta::apply<meta::quote<std::tuple>, has_init_t>;
        using has_bits_t = has_bitset_t;
        
        template <typename...HAS_LIST>
        constexpr has_bits_t hw_has_bitset(meta::list<HAS_LIST...>)
        {
            return { HAS_LIST::value... };
        }

        template <typename T>
        constexpr hw_init(T)
            : is_bits { detail::hw_isa_bitset<is_list, T>() }
            , has_tpl { T::value, meta::invoke<CP_DEFNS, T>::value...  }
                {}

        // XXX conversion from `tpl` to `bitset` is a problem.
        // XXX research static functions in dependent classes...
        constexpr auto& has_bits() const
        {
            return has_tpl;
        }

        detail::hw_bitset_t<is_list> is_bits;
        has_bits_t                   has_tpl;
    };
    
    static constexpr auto init = detail::init_list<hw_init>(is_list{});
    
    
    constexpr auto set(hw_tst t, bool value = true)
    {
        // logic error to set a `void` type
        if (t == hw_tst())
            throw std::logic_error("hw_defs::set: void invalid");

        if (t.feature() == 0) {
            // set CPU
            auto& defns  = init.at(t.cpu()-1);
            is_bits      = defns.is_bits;
            has_bits     = defns.has_bits();
            current_name = is_names[t.cpu()-1];
        } else {
            // set/clear FEATURE
            has_bits.at(t.cpu())[t.feature()-1] = value;
        }
#if 0
        std::cout << "hw_defs::set: name = " << current_name << std::endl;
        std::cout << "is_bits  = " << is_bits << std::endl;
        std::cout << "has_bits = " << has_bits[0] << std::endl;
        std::cout << "fpu_bits = " << has_bits[1] << std::endl;
        std::cout << "mmu_bits = " << has_bits[2] << std::endl;
#endif
    }

    auto clear(hw_tst t)
    {
        if (t.feature() == 0)
            throw std::logic_error("hw_defs::clear: can't clear CPU or void");
        return set(t, false);
    }
    
    const char *name()
    {
        return current_name;
    }
};

template <typename IS, typename...CP_DEFNS>
bool hw_defs<IS, CP_DEFNS...>::hw_tst::is_cp(hw_tst const& key) const
{
    static hw_tst cp_ids[] = { typename CP_DEFNS::key()... };
    
    if (key.cpu())
        return key.cpu() == cpu();

    for (auto& entry : cp_ids)
        if (key == entry)
            return true;

    return false;
}


template <typename IS, typename...CP_DEFNS>
const char *hw_defs<IS, CP_DEFNS...>::hw_tst::name() const
{
    if (feature() == 0)
        return cpu() ? is_names[cpu()-1] : "void";
    
    return has_names[cpu()][feature()-1];
}

}

#endif
