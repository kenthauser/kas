#ifndef KAS_PARSER_SYM_PARSER_DETAIL_H
#define KAS_PARSER_SYM_PARSER_DETAIL_H

namespace kas::parser::detail
{
    using namespace meta;
    using namespace meta::placeholders;
    
    // ADDER adds instances to string parser
    // default: simple adder: just add insn* using name() method 
    template <typename T, typename = void>
    struct adder
    {
        // parser return type
        using VALUE_T   = T const *;
        //using NAME_LIST = list<>;   // don't xlate any names

        template <typename PARSER>
        adder(PARSER) : insns(PARSER::sym_defns) {}

        template <typename X3>
        void operator()(X3& x3, unsigned count) const
        {
            auto& add = x3.add;
            for (auto p = insns; count--; ++p)
                add(p->name, p);
        }

        const T *insns;
    };

    // if T has ADDER member type, use as ADDER
    template <typename T>
    struct adder<T, std::void_t<typename T::ADDER>> : T::ADDER {}; 
    
    template <typename T, typename = void>
    struct name_list_impl
    {
        // default: no xlate
        using type = list<>;
    };

    // if ADDER has NAME_LIST member type, use it
    template <typename T>
    struct name_list_impl<T, std::void_t<typename T::NAME_LIST>>
    {
        using type = list<const char *, typename T::NAME_LIST>;
    };

    template <typename T>
    using name_list = _t<name_list_impl<T>>;

    // if ADDER has a XTRA_TYPES member type use it, else default to empty list
    template <typename T, typename = void>
    struct xtra_types_impl { using type = list<>; };

    template <typename T>
    struct xtra_types_impl<T, std::void_t<typename T::XTRA_TYPES>>
    {
        using type = typename T::XTRA_TYPES;
    };

    template <typename T>
    using xtra_types = _t<xtra_types_impl<T>>;

    // default to no xlate
    template <typename T, typename = void, typename = void>
    struct xlate_list_impl
    {
        using type = list<>;
    };
   
    // prefer `XLATE_LIST` member type if present
    template <typename T, typename U>
    struct xlate_list_impl<T, U, std::void_t<typename T::XLATE_LIST>>
    {
        using type = typename T::XLATE_LIST;
    };

    // otherwise use `name_list<T>` if not empty
    template <typename T>
    struct xlate_list_impl<T, std::void_t<at_c<name_list<T>, 0>>, void>
    {
        using type = list<name_list<T>>;
    };

    template <typename T>
    using xlate_list = _t<xlate_list_impl<T>>;


    template <typename T, typename LIST, typename TYPES = list<>, typename CTOR = void>
    struct init_from_list_impl
    {
        using type = init_from_list_impl<T, transform<LIST, invoke<CTOR, TYPES>>>;
    };

    template <typename T, typename = void>
    struct has_value : std::false_type {};

    template <typename T>
    struct has_value<T, std::void_t<decltype(T::value)>> : std::true_type {};

    template <typename T>
    constexpr auto value_of(T t)
    {
        // if T has `value` member, return that
        if constexpr (has_value<T>::value)
            return T::value;
        else
            return t;
    }

    template <typename T, typename...Ts, typename TYPES>
    struct init_from_list_impl<T, list<Ts...>, TYPES, void>
    {
        using type = init_from_list_impl;

        // NB: use std::array<> for support of zero-length arrays
        static constexpr auto size = sizeof...(Ts);
        static constexpr std::array<T, size> data { value_of(Ts())... };
        static constexpr auto value = data.data();
    };


    template <typename T, typename LIST, typename...Ts>
    using init_from_list = _t<init_from_list_impl<T, LIST, Ts...>>;

/////////////////////
    
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


template <typename TYPE_LIST, typename FN>
struct get_one_insn_types
{
    template <typename DEFN>
    using invoke = _t<invoke<FN, get_type_list<TYPE_LIST, DEFN>>>;
};


#ifndef USE_FOLD

template <typename DEFNS, typename TYPE_LIST, typename FN>
using get_all_insn_types = invoke<quote<join>, transform<DEFNS, get_one_insn_types<TYPE_LIST, FN>>>;

// callable to extract type list for one item
template <typename DEFNS>
struct get_types_one_item
{

    template <typename RT, typename TYPE_LIST, typename CTOR = void, typename XTRA = list<>, typename FN = quote<id>>
    using invoke = _t<unique<concat<get_all_insn_types<DEFNS, TYPE_LIST, FN>, XTRA>>>;
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
    template <typename RT, typename TYPE_LIST, typename CTOR = void, typename XTRA = list<>, typename FN = quote<id>>
    using invoke = _t<fold<DEFNS, XTRA, get_all_types_impl<TYPE_LIST, FN>>>;
};
#endif
    

template <typename XLATE_LIST, typename DEFNS>
using get_all_types = transform<XLATE_LIST, uncurry<get_types_one_item<DEFNS>>>;

/////////////////////

template <typename DEFN, typename TYPE>
struct xlate_one_rule
{
    template <typename RT, typename TYPE_LIST, typename CTOR = void, typename XTRA = list<>, typename FN = quote<id>>
    using invoke = _t<transform<invoke<get_one_insn_types<TYPE_LIST, FN>, DEFN>
                              , bind_front<quote<find_index>, TYPE>>>;
};

template <typename DEFN>
struct xlate_one_defn_impl
{
    template <typename RULE, typename TYPE>
    using invoke = _t<apply<xlate_one_rule<DEFN, TYPE>, RULE>>;
};


template <typename X_LIST, typename X_TYPES>
struct xlate_one_defn
{
    template <typename DEFN>
    using invoke = _t<transform<X_LIST, X_TYPES, xlate_one_defn_impl<DEFN>>>;
};

/////////////////////

template <typename T, typename XLATE_LIST, typename = void, typename = void>
struct ctor_impl { using type = void; };

template <typename T, typename XLATE_LIST>
struct ctor_impl <T, XLATE_LIST, std::void_t<typename T::CTOR>, void> : id<typename T::CTOR> {};

// pickup XLATE_LIST here. TYPES is `invoked` on trait just before xlate
template <typename T, typename XLATE_LIST, typename U>
struct ctor_impl <T, XLATE_LIST, U, std::void_t<front<XLATE_LIST>>>
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

template <typename T>
using ctor = _t<ctor_impl<T, xlate_list<T>>>;

/////////////////////

struct gen_all_defns_impl
{

    template <typename TYPE_LIST>
    struct gen_one_defns
    {
        template <typename RT, typename TL, typename CTOR = void, typename...>
        using invoke = init_from_list<RT, TYPE_LIST, list<>, CTOR>;
    };

    template <typename XLATE, typename TYPE_LIST>
    using invoke = _t<if_<std::is_void<front<XLATE>>, id<void>
                        , lazy::apply<gen_one_defns<TYPE_LIST>, XLATE>
                        >>;
};


template <typename XLATE_LIST, typename TYPES>
using gen_all_defns = transform<XLATE_LIST, TYPES, gen_all_defns_impl>;

/////////////////////
}

#endif

