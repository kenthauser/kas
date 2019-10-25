#ifndef KAS_DEFN_UTILS_H
#define KAS_DEFN_UTILS_H

#include "kas_string.h"
#include <meta/meta.hpp>

namespace kas
{

// XXX is this comment correct?
// using integer sequences, not MPL::strings, for name definition
// bundle character sequences into a type
//    template <int...NAME>
//    using IS = std::integer_sequence<int, NAME...>;

using string::kas_string;
using string::str_cat;

// test if type is meta::list<>
template <typename T, typename = void>
struct is_meta_list : std::false_type {};

template <typename...Ts>
struct is_meta_list<meta::list<Ts...>, void> : std::true_type {};

// name `modules` in canonical order
// `defn_tags` used by configure.py to build machine includes
using defn_tags = meta::list<
          struct defn_expr
        , struct defn_parser
        , struct defn_core
        , struct defn_fmt
        , struct defn_host
        , struct defn_cpu
        >;

// XXX should this be `meta::fold`
template <template<typename=void> class T, typename LIST=defn_tags>
using all_defns = meta::join<
        meta::transform<
            LIST
          , meta::quote_trait<T>
          >
        >;


namespace detail
{
    using namespace meta;

    template <typename FN, typename STATE, typename Item>
    struct do_flatten
    {
        using type = push_back<STATE, invoke<FN, Item>>;
    };

    template <typename List, typename FN, typename STATE = list<>>
    using defn_flatten = _t<fold<List
                            , STATE
                            , bind_front<quote_trait<do_flatten>, FN>>>;
    
    template <typename FN, typename STATE, typename...Items>
    struct do_flatten<FN, STATE, meta::list<Items...>>
    {
        using type = _t<defn_flatten<meta::list<Items...>, FN, STATE>>;
    };
}

// `flatten` commonly used to join target insn sublists into single list
template <template<typename=void> class T, typename LIST=kas::defn_tags, 
           typename FN = meta::quote<meta::id_t>>
using all_defns_flatten = meta::_t<detail::defn_flatten<meta::transform<LIST
                                             , meta::quote_trait<T>>, FN>>;

#if 0
// XXX obsolete
template <template<typename> class T
        , template<typename...> class T2
        , typename LIST=defn_tags>
using traits2types = meta::transform<
          all_defns<T, LIST>
        , meta::uncurry<meta::quote<T2>>
        >;

#endif
}

#endif
