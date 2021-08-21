#ifndef KAS_BSD_BSD_ARG_DEFN_H
#define KAS_BSD_BSD_ARG_DEFN_H

#include "parser/token_defn.h"

namespace kas::bsd
{

using parser::token_defn_t;

// declare all tokens used in parsing BSD pseudo ops
using tok_bsd_ident         = token_defn_t<KAS_STRING("IDENT")
                                         , core::core_symbol_t>;

using tok_bsd_local_ident   = token_defn_t<KAS_STRING("L_IDENT")
                                         , core::core_symbol_t>;

using tok_bsd_numeric_ident = token_defn_t<KAS_STRING("N_IDENT") 
                                         , core::core_symbol_t>;

using tok_bsd_dot           = token_defn_t<KAS_STRING("DOT")
                                         , core::core_addr_t>;

using tok_bsd_at_num        = token_defn_t<KAS_STRING("AT_NUM")
                                         , unsigned>;

using tok_bsd_at_ident      = token_defn_t<KAS_STRING("AT_IDENT")>;

// XXX obsolete definition? just address as `token`?
using bsd_arg  = parser::kas_token;
using bsd_args = std::vector<bsd_arg>;

}

namespace kas::parser
{
// Declare `token_defn_t` types with specialized `gen_data_p`
template <> void const *bsd::tok_bsd_ident::
    gen_data_p(kas_token const&, std::type_info const&, void const *) const;
template <> void const *bsd::tok_bsd_local_ident::
    gen_data_p(kas_token const&, std::type_info const&, void const *) const;
template <> void const *bsd::tok_bsd_numeric_ident::
    gen_data_p(kas_token const&, std::type_info const&, void const *) const;
template <> void const *bsd::tok_bsd_dot::
    gen_data_p(kas_token const&, std::type_info const&, void const *) const;
}

#endif

