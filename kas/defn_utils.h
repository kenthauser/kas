#ifndef KAS_DEFN_UTILS_H
#define KAS_DEFN_UTILS_H

#include "kas_string.h"
#include <meta/meta.hpp>

namespace kas
{

// XXX `kas_defn_tags` should move to `kas` proper
// name `modules` in canonical order
using kas_defn_tags = meta::list<
          struct defn_expr
        , struct defn_core
        , struct defn_fmt
        , struct defn_host
        , struct defn_cpu
        , struct defn_parser
        >;

// `transform` creates lists. `join` combines lists
template <template<typename> class T, typename TAG_LIST = kas_defn_tags>
using all_defns = meta::join<
                    meta::transform<
                        TAG_LIST
                      , meta::quote_trait<T>
                      >
                    >;


namespace detail
{
    using namespace meta;

    // if `Item` is not a `meta::list`, append to result
    template <typename FN, typename STATE, typename Item>
    struct do_flatten
    {
        using type = push_back<STATE, invoke<FN, Item>>;
    };

    // driver for `do_flatten`. Must be defined before `list` specialization
    template <typename List, typename FN, typename STATE = list<>>
    using defn_flatten = _t<fold<List
                               , STATE
                               , bind_front<quote_trait<do_flatten>, FN>>>;
    
    // if `Item` is a `meta::list`, then recurse
    template <typename FN, typename STATE, typename...Items>
    struct do_flatten<FN, STATE, list<Items...>>
    {
        using type = _t<defn_flatten<list<Items...>, FN, STATE>>;
    };

}

// `flatten` commonly used to join target insn sublists into single list
template <template<typename> class T, typename LIST, typename FN = meta::quote<meta::id_t>>
using all_defns_flatten = meta::_t<detail::defn_flatten<
                                            meta::transform<LIST
                                                          , meta::quote_trait<T>
                                                          >
                                          , FN>
                                  >;

}

#endif
