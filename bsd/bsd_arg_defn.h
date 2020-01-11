#ifndef KAS_BSD_BSD_ARG_DEFN_H
#define KAS_BSD_BSD_ARG_DEFN_H

//#include "parser/parser_types.h"
#include "parser/token_defn.h"
#include <ostream>

namespace kas::bsd
{

using parser::token_defn_t;

using tok_bsd_ident         = token_defn_t<KAS_STRING("IDENT")   , core::symbol_ref>;
using tok_bsd_local_ident   = token_defn_t<KAS_STRING("L_IDENT") , core::symbol_ref>;
using tok_bsd_numeric_ident = token_defn_t<KAS_STRING("N_IDENT") , core::symbol_ref>;
using tok_bsd_dot           = token_defn_t<KAS_STRING("DOT")     , core::addr_ref>;
using tok_bsd_at_num        = token_defn_t<KAS_STRING("AT_NUM")  , unsigned>;
using tok_bsd_at_ident      = token_defn_t<KAS_STRING("AT_IDENT")>;
using tok_bsd_missing       = token_defn_t<KAS_STRING("MISSING")>;

// XXX obsolete?
using bsd_arg  = parser::kas_token;
using bsd_args = std::vector<bsd_arg>;

}

namespace kas::parser
{
    // Declare `token_defn_t` types with specialized `gen_epr`

    template <> void bsd::tok_bsd_ident::gen_expr(expr_t&, kas_token const&) const;
}

#endif

