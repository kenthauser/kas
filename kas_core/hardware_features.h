#ifndef KAS_KAS_CORE_HARDWARE_FEATURES_H
#define KAS_KAS_CORE_HARDWARE_FEATURES_H

// Metafunction to assist with hardware feature definitions.

// This metafunction takes 5 types as follows:
//
// hw_features<FTL, NAME, BASES, ADD, DEL>
//
// where:
//
//  FTL   = a meta::list<> of all supported features
//  NAME  = a KAS_STRING<> for this definition
//  BASES = a meta::list<> of base `hw_feature` classes
//          to inheirit defns from
//  ADD   = a meta::list<> of features (each must be in FTL)
//          to set for this definition
//  DEL   = a meta::list<> of features to delete.
//
// the resulting type has the following member types and constants
//
// type     = metafunction result (which inherits from `all` BASES)
// ftl      = FTL
// name     = NAME
// value    = `unsigned type` with bits set (according to index in FTL)
//            as result of adding/deleting. 
//          
// Notes:
//   DEL is sticky. once a feature is deleted, it can't be restored.
//   if DEL<> is omitted, ADD<> can be variadic list
//   single BASE doesn't have to be wrapped in a meta::list<>.
//

/************************************************

// Example code from `m68k`

// create alias for cpu feature definitons
template <typename...Args>
using cpu_defn = hw_features<cpu_feature_types, Args...>;


// base instructions & address modes supported by m68000 & coldfire ISA_A
// `limit_3w` limits instructions to three words total (coldfire)
using m68kcf = cpu_defn<STR("base"), list<>, limit_3w>;

// enable m68k address modes not supported by coldfire
using m68k   = cpu_defn<STR("m68k"), m68kcf
                    , list<index_word, movep>   // added features
                    , list<limit_3w>            // deleted features
                    >;

// declare processor hierarchy: features typically inhierited after introduction
using m68000 = cpu_defn<STR("m68000"), m68k>;
using m68010 = cpu_defn<STR("m68010"), m68000, read_ccr>;

using m68020 = cpu_defn<STR("m68020"), m68010
                    // '020 added lots of features
                    , branch_long
                    , callm                     // 68020 only
                    , mult64
                    , index_full
                    , index_scale
                    , index_scale_8
                    >;

using m68030 = cpu_defn<STR("m68030"), m68020
                    , list<mmu>                 // added features
                    , list<callm>               // deleted features
                    >;

using m68040 = cpu_defn<STR("m68040"), m68030, fpu>;
using m68060 = cpu_defn<STR("m68060"), m68040
                    , list<>
                    , list<movep>               // deleted feature
                    >;

// add reduced feature set types
using m68ec000 = cpu_defn<STR("m68EC000"), m68000, read_ccr>;

using m68lc040 = cpu_defn<STR("m68LC040"), m68040, list<>, list<fpu>>;
using m68ec040 = cpu_defn<STR("m68EC040"), m68lc040, list<>, list<mmu>>;

using m68lc060 = cpu_defn<STR("m68LC060"), m68060, list<>, list<fpu>>;
using m68ec060 = cpu_defn<STR("m68EC060"), m68lc060, list<>, list<mmu>>;

// cpu32 derived from EC000, not 68000
using cpu32    = cpu_defn<STR("cpu32"), m68ec000, table>;

// coldfire supports index scaled address mode and an multiplying accumulator
using coldfire = cpu_defn<STR("coldfire"), m68kcf
                    , read_ccr
                    , index_scale
                    , mult64
                    , mac
                    , fpu
                    , table
                    , mmu
                    >;
     
************************************************/

#include <type_traits>
#include <meta/meta.hpp>

namespace kas::core::hardware
{
namespace detail {
    using namespace meta;

    // limit of 64 features. (std::bitset<> can handle 64 bits)
    template <std::size_t N = 32>
    using calc_bitset_t = std::conditional_t<N <=  8, uint8_t,
                          std::conditional_t<N <= 16, uint16_t,
                          std::conditional_t<N <= 32, uint32_t,
                          std::conditional_t<N <= 64, uint64_t,
                                                      void>>>>; 

    template <typename FTL>
    using hw_bitset_t = calc_bitset_t<FTL::size()>;

    // NB: meta-programs often define terminals first...enables auto
    // meta-program terminal: return accumulated value
    template <typename FTL>
    auto constexpr hw_bitset(hw_bitset_t<FTL> N = 0)
    {
        return N;
    }
    
    // meta-program: add index of type `T` to the bitset & recurse.
    template <typename FTL, typename T, typename...Rest>
    auto constexpr hw_bitset(hw_bitset_t<FTL> N = 0)
    {
        using I = find_index<FTL, T>;
        static_assert(!std::is_same_v<I, npos>, "Invalid feature");
        return hw_bitset<FTL, Rest...>(N | (1 << I::value));
    }

    // declare (unimplemented) implementation base template
    template <typename...> struct hw_feature_impl;

    // Actual trait type implementation
    template <typename FTL      // feature type list: list<> of all 'has' features
            , typename NAME     // name of new type
            , typename...BASE   // base types
            , typename...ADD    // features to add
            , typename...DEL    // features to delete
            >
    struct hw_feature_impl<FTL, NAME, list<BASE...>, list<ADD...>, list<DEL...>>
            : BASE...           // derive from BASE... to get `is_a` relationship
    {
        using type = hw_feature_impl;
        using ftl  = FTL;
        using name = NAME;

        // accumulate value from base classes
        // XXX why are paren's requried? 2018/04/01 clang compile error KBH
        static constexpr auto add_v = (hw_bitset<FTL, ADD...>() | ... | BASE::add_v);
        static constexpr auto del_v = (hw_bitset<FTL, DEL...>() | ... | BASE::del_v);

        // public interface: NB: del is sticky
        static constexpr auto value = add_v &~ del_v;
    };

    // declare (unimplemented) base template for trait to describe HW feature
    template <typename...> struct hw_features;
    
    // specialize `hw_feature` template to facilitate use
    // single base with variadic arglist for ADD list, no DEL
    template <typename FTL, typename NAME, typename BASE, typename...ADD>
    struct hw_features<FTL, NAME, BASE, ADD...>
            : hw_feature_impl<FTL, NAME, list<BASE>, list<ADD...>, list<>> {};

    // list base with variadic arglist for ADD list, no DEL
    template <typename FTL, typename NAME, typename...BASE, typename...ADD>
    struct hw_features<FTL, NAME, list<BASE...>, ADD...>
            : hw_feature_impl<FTL, NAME, list<BASE...>, list<ADD...>, list<>> {};

    // single base with add & del lists
    template <typename FTL, typename NAME, typename BASE, typename...ADD, typename...DEL>
    struct hw_features<FTL, NAME, BASE, list<ADD...>, list<DEL...>>
          : hw_feature_impl<FTL, NAME, list<BASE>, list<ADD...>, list<DEL...>> {};

    // list base with add & del lists
    template <typename FTL, typename NAME, typename...BASE, typename...ADD, typename...DEL>
    struct hw_features<FTL, NAME, list<BASE...>, list<ADD...>, list<DEL...>>
          : hw_feature_impl<FTL, NAME, list<BASE...>, list<ADD...>, list<DEL...>> {};
}

using detail::hw_features;
}
#endif

