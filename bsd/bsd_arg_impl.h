#ifndef KAS_BSD_BSD_ARG_IMPL_H
#define KAS_BSD_BSD_ARG_IMPL_H

#include "bsd_arg_defn.h"
#include "bsd_symbol.h"

namespace kas::parser
{

// NB: all of this is in namespace `kas::parser`
template <> void const *bsd::tok_bsd_ident
        ::gen_data_p(kas_token const& tok) const
{
    return &bsd::bsd_ident::get(tok);
}

template <> void const *bsd::tok_bsd_local_ident
        ::gen_data_p(kas_token const& tok) const
{
    // XXX form: references `tok_fixed`
    return &bsd::bsd_local_ident::get(tok._fixed, tok);
}

template <> void const *bsd::tok_bsd_numeric_ident
        ::gen_data_p(kas_token const& tok) const
{
    return &bsd::bsd_numeric_ident::get(tok);
}

}

#endif


