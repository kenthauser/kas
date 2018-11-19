#ifndef KAS_PARSER_OP_PARSER_H
#define KAS_PARSER_OP_PARSER_H

namespace kas::parser

{

namespace detail
{
    using namespace meta;
    using namespace meta::placeholders;
    
    // ADDER adds instances to string parser
    // default: simple adder: just add insn* using name() member
    template <typename T, typename = void>
    struct adder
    {
        // parser return type
        using VALUE_T   = T const *;
        using NAME_LIST = list<>;   // don't xlate any names

        template <typename PARSER>
        adder(PARSER) : insns(PARSER::insns) {}

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
        // default: only first type in list
        using type = list<int_<0>>;
    };

    // if ADDER has NAME_LIST member type, use it
    template <typename T>
    struct name_list_impl<T, std::void_t<typename T::NAME_LIST>>
    {
        using type = typename T::NAME_LIST;
    };

    template <typename T>
    using name_list = _t<name_list_impl<T>>;

    // if ADDER has a XTRA_NAMES member type, else default to empty list
    template <typename T, typename = void>
    struct xtra_names_impl { using type = list<>; };

    template <typename T>
    struct xtra_names_impl<T, std::void_t<typename T::XTRA_NAMES>>
    {
        using type = typename T::XTRA_NAMES;
    };

    template <typename T>
    using xtra_names = _t<xtra_names_impl<T>>;

    // default to just `name_list`
    template <typename T, typename = void>
    struct xlate_list_impl
    {
        using type = list<name_list<T>>;
    };

    template <typename T>
    struct xlate_list_impl<T, std::void_t<typename T::XLATE_LIST>>
    {
        using type = typename T::XLATE_LIST;
    };

    template <typename T>
    using xlate_list = _t<xlate_list_impl<T>>;


    template <typename T, typename LIST, typename NAMES = list<>, typename CTOR = void>
    struct init_from_list_impl
    {
        using type = init_from_list_impl<T, transform<LIST, invoke<CTOR, NAMES>>>;
    };

    template <typename T, typename...Ts, typename NAMES>
    struct init_from_list_impl<T, list<Ts...>, NAMES, void>
    {
        using type = init_from_list_impl;

        // NB: use std::array<> for support of zero-length arrays
        static constexpr auto size = sizeof...(Ts);
        static constexpr std::array<T, size> data { Ts()... };
        static constexpr auto value = data.data();
    };

    template <typename T, typename LIST, typename...Ts>
    using init_from_list = _t<init_from_list_impl<T, LIST, Ts...>>;
#define USE_FOLD
    // extract INDEX type from DEFN, returning void is out of range
    template <typename DEFN>
    struct get_one_name
    {
        template <typename INDEX>
        using invoke = _t<if_c<INDEX::value < DEFN::size()
                             , lazy::at<DEFN, INDEX>
                             , id<void>
                             >>;
    };
#ifndef USE_FOLD
    // NB: use fold to combine tranform and filter in one pass
    // Unoptimized version:
    template <typename NAME_LIST, typename DEFN>
    using get_names = filter<transform<NAME_LIST, get_one_name<DEFN>>
                           , not_fn<quote<std::is_void>>
                           >;
#else
    template <typename DEFN>
    struct get_names_impl
    {
        template <typename STATE, typename INDEX>
        using do_push_back = push_back<STATE, at<DEFN, INDEX>>;
        
        template <typename STATE, typename INDEX>
        using invoke = _t<if_c<INDEX::value < DEFN::size()
                             , defer<do_push_back, STATE, INDEX>
                             , STATE
                             >>;

    };

    template <typename NAME_LIST, typename DEFN>
    using get_names = fold<NAME_LIST, list<>, get_names_impl<DEFN>>;
#endif

#ifndef USE_FOLD
    template <typename DEFNS, typename NAME_LIST>
    using get_insn_names = transform<DEFNS, bind_front<quote<get_names>, NAME_LIST>>;

    // NB use fold to combine unique, join, & concat in one pass
    template <typename DEFNS, typename NAME_LIST, typename XTRA_NAMES = list<>>
    using get_all_names = unique<concat<join<get_insn_names<DEFNS, NAME_LIST>>, XTRA_NAMES>>;
#else
    template <typename NAME_LIST>
    struct get_all_names_impl
    {
        template <typename DEFN>
        struct get_one_defn
        {
            template <typename STATE, typename INDEX>
            using do_push_unique = _t<meta::detail::insert_back_<STATE, at<DEFN, INDEX>>>;
            
            template <typename STATE, typename INDEX>
            using invoke = _t<if_c<INDEX::value < DEFN::size()
                                 , defer<do_push_unique, STATE, INDEX>
                                 , STATE
                                 >>;
        };

        template <typename STATE, typename DEFN>
        using invoke = fold<NAME_LIST, STATE, get_one_defn<DEFN>>;
    };

    template <typename DEFNS, typename NAME_LIST, typename XTRA_NAMES = list<>>
    using get_all_names = fold<DEFNS, XTRA_NAMES, get_all_names_impl<NAME_LIST>>;
#endif


    template <typename T, typename NL, typename = void, typename = void>
    struct ctor_impl { using type = void; };

    template <typename T, typename NL>
    struct ctor_impl <T, NL, std::void_t<typename T::CTOR>, void> : id<typename T::CTOR> {};

    // pickup NL here. NAMES is `invoked` on trait just before xlate
    template <typename T, typename NL, typename U>
    struct ctor_impl <T, NL, U, std::void_t<front<NL>>>
    {
        template <typename NAMES>
        struct xlate
        {
            // when `quoted_trait` is `invoked` to supply NAMES
            using type = xlate;
            
            // xlate `name_list` indexes of defn thru `NAMES`
            template <typename DEFN>
            using xlate_names = transform<get_names<NL, DEFN>
                                        , bind_front<quote<find_index>, NAMES>
                                        >;

            // prepend xlated names to DEFN list
            template <typename DEFN>
            using invoke = list<xlate_names<DEFN>
                              , DEFN
                              >;
        };
        
        // `NAMES` is `invoked` just before calculation
        using type = quote_trait<xlate>;
    };
    
    template <typename T>
    using ctor = _t<ctor_impl<T, front<xlate_list<T>>>>;
}

// base template: specialize for various use cases
template <typename...> struct op_parser_t;

// Simple case: use name member to return T const*. Use default ADDER
template <typename T, typename DEFNS>
struct op_parser_t<T, DEFNS> :
        op_parser_t<T, DEFNS, detail::adder<T>> {};

// Advanced case: specify ADDER, use default NAMES
template <typename T, typename DEFNS, typename ADDER>
struct op_parser_t<T, DEFNS, ADDER> :
        op_parser_t<T, DEFNS, ADDER, void> {};

// General case: specify ADDER and optional NAMES
template <typename T, typename DEFNS, typename ADDER, typename NAMES>
struct op_parser_t<T, DEFNS, ADDER, NAMES>
{
    using Encoding = boost::spirit::char_encoding::standard;
    using x3_parser_t = x3::symbols_parser<Encoding, typename ADDER::VALUE_T>;

    void add() const
    {
        if (!parser)
            parser = new x3_parser_t;
        ADDER{*this}(*parser, insns_cnt);
    }

    // x3::symbols_parser is a "prefix" parser
    // most parsers need to be wrapped in lexeme[x3 >> !alnum].
    // operators are the exception. do at instantiation
    auto& x3_raw() const
    {
        if (!parser)
            add();
        return *parser;
    }

    auto x3() const
    {
        return x3::lexeme[x3_raw() >> !x3::alnum];
    }

    struct deref
    {
        template <typename Context>
        void operator()(Context& ctx)
        {
            x3::_val(ctx) = *x3::_attr(ctx);
        };
    };
    
    auto x3_deref() const
    {
        return x3()[deref()];
    }

    static inline x3_parser_t *parser;

    // meta programs to generate compile-time names/defns
    using name_list  = detail::name_list<ADDER>;
    using xtra_names = detail::xtra_names<ADDER>;
    using xlate_list = detail::xlate_list<ADDER>;

    using all_names  = meta::if_<std::is_void<NAMES>
                               , detail::get_all_names<DEFNS, name_list, xtra_names>
                               , NAMES
                               >;
    static constexpr auto names = detail::init_from_list<const char *, all_names>::value;

    using ctor   = detail::ctor<ADDER>;
    using defn_t = detail::init_from_list<T, DEFNS, all_names, ctor>;

    static constexpr auto insns     = defn_t::value;
    static constexpr auto insns_cnt = defn_t::size;
};

}

#endif

