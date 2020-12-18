#ifndef KAS_PARSER_SYM_PARSER_XLATE_H
#define KAS_PARSER_SYM_PARSER_XLATE_H

//
// XLATE_LIST: list of translations to make on `TYPES`
//
// Each item in XLATE_LIST describes a single "translation" to make on `TYPES` list.
// The XLATE_LIST can have mutiple "translations" to make. The translated list will
// have an entry for each `XLATE_LIST` item for each `TYPE`.
//
// For example the instruction definition `XLATE_LIST` has 4 items on the list:
// NAME, FORMATTER, VALIDATOR, and VAL_COMBO. If there are 100 instructions described
// in `TYPES`, the result of translation with yield a `meta::list` 100 items long.
// Each type in the `meta::list` will be another `meta::list` with 4 items, 1 for each
// translation in the `XLATE_LIST`.
//
// The allowed translations are all designed to facilitate generation of `constexpr`
// definitions for type-lists describing instructions & registers. The supported
// "translations" are:
//
// 1. convert type to index. 
//    This translation takes all types at the specified index(es) in `TYPES`
//    (think name), generates a master list with all duplicates removed, 
//    and return index(es) into that list.
//    
// 2. convert virtual-type to index
//    Similar to above, but use special `virtual-type` constructor to build master-list
//    of types which hold pointers to instantiated instances. Return index(es) into
//    that list
//
// 3. convert "index-list" into index.
//    Special for "validator lists". Take `meta::list` of validators for each
//    instruction, generate master list of `lists` with duplicates removed &
//    return index into that list.
//
// The conversion is performed in three steps.
// First, the "master lists" of types are extracted into `all_types` by
//    metafunction `detail::get_all_types`
//
// Second, all of the types in the master list are instantiated. The result is a
//    `std::array` of instantiated values or, in the case of virtual-base types,
//    an array of pointers to instantiated derived classes. This operation is
//    performed by metafunction `detail::gen_all_defns`.
//      
// Third, the list of "indexs" is generated using master generated in first
//    step by metafunction. This `xlated` index is passed to the constructor
//    of type `T` along with the untranslated `DEFN`. For each type in the
//    `meta::list` DEFN (passed to `sym_parser_t` as template arg) (call it Dn)
//    type `T` is instantiated as `T(meta::list<Dn, Xn>())` where Xn is the
//    named `Dn` types converted to indexes in `std::integral_constant`s.
//    This operation is performed by metafunction `ctor`.
//
// The result of step 3 is stored as `sym_defns` in the parser.
//

#include "kas/init_from_list.h"

namespace kas::parser::detail
{
using namespace meta;
    
// extend `meta::at`: if out-of-range, return void
template <typename DEFN>
struct get_one_type_index
{
    template <typename INDEX>
    using invoke = _t<if_c<INDEX::value < DEFN::size()
                         , lazy::at<DEFN, INDEX>
                         , id<void>
                         >>;
};

// large type-lists require use of `fold` instead of convenience methods
#define USE_FOLD
#ifndef USE_FOLD
// NB: use fold to combine tranform and filter in one pass
// Unoptimized version:
template <typename TYPE_LIST, typename DEFN>
using get_type_list = _t<filter<transform<TYPE_LIST, get_one_type_index<DEFN>>
                              , not_fn<quote<std::is_void>>
                              >>;

#else
template <typename DEFN>
struct get_type_list_impl
{
    template <typename STATE, typename INDEX>
    using do_push_back = push_back<STATE, at<DEFN, INDEX>>;
    
    template <typename STATE, typename INDEX>
    using invoke = _t<if_c<INDEX::value < DEFN::size()
                         , defer<do_push_back, STATE, INDEX>
                         , STATE
                         >>;

};

template <typename TYPE_LIST, typename DEFN, typename XTRA = list<>>
using get_type_list = fold<TYPE_LIST, XTRA, get_type_list_impl<DEFN>>;
#endif

/////////////////////

template <typename NAME_LIST, typename = void>
struct get_fn : quote_trait<list> {};

template <typename NAME_LIST>
struct get_fn<list<NAME_LIST>, void> : quote_trait<id> {};

// if NAME_LIST mentions more than one type, wrap in `list`
template <typename NAME_LIST, typename FN, typename FN_XXX = quote<list>>
struct get_one_insn_types
{
    //using FN_XXX = _t<if_c<NAME_LIST::size() == 1, quote<id>, quote<list>>>;
    //static_assert(std::is_same_v<FN_XXX, FN>);
    template <typename DEFN>
    using invoke = _t<invoke<FN, get_type_list<NAME_LIST, DEFN>>>;
    //using invoke = _t<invoke<get_fn<NAME_LIST>, get_type_list<NAME_LIST, DEFN>>>;
};

template <typename NAME_LIST, typename FN>
struct get_one_insn_types<list<NAME_LIST>, FN> : 
            get_one_insn_types<list<NAME_LIST>, FN, quote<id>> {};

#ifndef USE_FOLD

template <typename DEFNS, typename NAME_LIST, typename FN>
using get_all_insn_types = invoke<quote<join>, transform<DEFNS, get_one_insn_types<NAME_LIST, FN>>>;

// callable to extract type list for one item
template <typename DEFNS>
struct get_types_one_item
{

    template <typename RT
            , typename NAME_LIST
            , typename CTOR     = void
            , typename XTRA     = list<>
            , typename FN       = quote<id>>
    using invoke = _t<unique<concat<get_all_insn_types<DEFNS, NAME_LIST, FN>, XTRA>>>;
};


/////////////////////


#else

template <typename TYPE_LIST, typename FN>
struct get_all_types_impl
{
    template <typename DEFN>
    using get_one_insn_types = _t<invoke<FN, get_type_list<TYPE_LIST, DEFN>>>;

    template <typename STATE, typename T>
    using push_unique = _t<meta::detail::insert_back_<STATE, T>>;

    template <typename STATE, typename DEFN>
    using invoke = _t<fold<get_one_insn_types<DEFN>, STATE, quote<push_unique>>>;

};

template <typename DEFNS>
struct get_types_one_item
{
    template <typename RT
            , typename TYPE_LIST
            , typename CTOR     = void
            , typename XTRA     = list<>
            , typename FN       = quote<id>>
    using invoke = _t<fold<DEFNS, XTRA, get_all_types_impl<TYPE_LIST, FN>>>;
};
#endif
    

template <typename XLATE_LIST, typename DEFNS>
using get_all_types = transform<XLATE_LIST, uncurry<get_types_one_item<DEFNS>>>;

/////////////////////

template <typename DEFN, typename TYPE>
struct xlate_one_rule
{
    template <typename RT
            , typename NAME_LIST
            , typename CTOR      = void
            , typename XTRA      = list<>
            , typename FN        = quote<id>>
    using invoke = _t<transform<invoke<get_one_insn_types<NAME_LIST, FN>, DEFN>
                              , bind_front<quote<find_index>, TYPE>>>;
};

template <typename DEFN>
struct xlate_one_defn_impl
{
    template <typename RULE, typename TYPE>
    using invoke = _t<apply<xlate_one_rule<DEFN, TYPE>, RULE>>;
};


template <typename X_LIST, typename X_NAMES>
struct xlate_one_defn
{
    template <typename DEFN>
    using invoke = _t<transform<X_LIST, X_NAMES, xlate_one_defn_impl<DEFN>>>;
};

/////////////////////
//
// `gen_all_defns`
//
// apply the `XLATE_LIST` to `ALL_TYPES`
// NB: ALL_TYPES holds one `list` for each item in `XLATE_LIST`
//
// Each item in the list is then passed to `init_from_list` to generate
// constexpr defns for each `XLATE_LIST` item.
//
/////////////////////

struct gen_all_defns_impl
{

    template <typename TYPES>
    struct gen_one_defns
    {
        template <typename RT
                , typename NAME_LIST
                , typename CTOR     = void
                , typename XTRA     = list<>
                , typename...>
        using invoke = init_from_list<RT, TYPES, CTOR, XTRA>;
    };

    // if `type` of defn is `void`, don't generate defn for this XLATE_LIST item.
    // NB: currently used by `tgt_val_c`
    template <typename XLATE, typename TYPES>
    using invoke = _t<if_<std::is_void<front<XLATE>>
                        , id<void>
                        , lazy::apply<gen_one_defns<TYPES>, XLATE>
                        >>;
};


template <typename XLATE_LIST, typename ALL_TYPES>
using gen_all_defns = transform<XLATE_LIST, ALL_TYPES, gen_all_defns_impl>;

/////////////////////
//
// generate CTOR for `sym_parser_t::defns_t`
//
// the `CTOR` is used (along with `all_types`) for the call to 
// metafunction `init_from_list` for the definitions passed to `sym_parser_t`.
//
// If a `XLATE_LIST` is in use (either from ADDER::XLATE_LIST, or from T::NAME_LIST)
// the definitions must be translated before the constexpr array is generated.
//
// If no XLATE_LIST in use, just use simplified version of `init_to_list`
//
// If a specialized `CTOR` is mentioned in `ADDER` use it.
//
// Otherwise for each type `T` in `defns_t` a `meta::list` is generated with
// two type: The first type is the `meta::list` with one `meta::list` for each entry in
// XLATE_LIST. The second type in the `list` is the original type-list for the instance.
// This two-element `meta::list` is passed to a constexpr constructor of `T` to generate
// defnition stored in `defn_t`
//
// As a special case, the the XLATE_LIST has a single entry (as does the XLATE_LIST
// automatically generated for types with `NAME_LIST`), then the first type of
// the constructor argument will not be a `list<list<...>,...>` but `list<...>`
//
/////////////////////


// ADDER with XLATE_LIST here. TYPES is `invoked` on trait just before xlate
template <typename ADDER, typename XLATE_LIST, typename = void>
struct ctor_impl
{
    template <typename TYPES>
    struct xlate
    {
        // when `quoted_trait` is `invoked` to supply TYPES
        using type = xlate;
        
        // xlate `name_list` indexes of defn thru `TYPES`
        template <typename DEFN>
        using xlated_defn = _t<invoke<xlate_one_defn<XLATE_LIST, TYPES>, DEFN>>;

        template <typename DEFN>
        using defn_prefix = if_c<xlated_defn<DEFN>::size() == 1
                                , front<xlated_defn<DEFN>>
                                , xlated_defn<DEFN>
                                >;

        // prepend xlated types to DEFN list
        template <typename DEFN>
        using invoke = _t<list<defn_prefix<DEFN>, DEFN>>;
    };
    
    // `TYPES` is `invoked` just before calculation
    using type = quote_trait<xlate>;
};

// if specialized `CTOR` in `ADDER`, use it.
template <typename ADDER, typename XLATE_LIST>
struct ctor_impl <ADDER, XLATE_LIST, std::void_t<typename ADDER::CTOR>>
                 : id<typename ADDER::CTOR> {};

// if no XLATE_LIST nor spcialized CTOR, just normal `init_from_list`
template <typename ADDER>
struct ctor_impl<ADDER, list<>, void> : id<void> {};

template <typename ADDER>
using ctor = _t<ctor_impl<ADDER, xlate_list<ADDER>>>;
}

#endif

