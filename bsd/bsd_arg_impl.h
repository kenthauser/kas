#ifndef KAS_BSD_BSD_ARG_IMPL_H
#define KAS_BSD_BSD_ARG_IMPL_H

#include "bsd_arg_defn.h"
#include "bsd_symbol.h"

namespace kas::parser
{

// NB: all of this is in namespace `kas::parser`

template <> void bsd::tok_bsd_ident
        ::gen_expr(expr_t& e, kas_token const& pos) const
{
    std::cout << "tok_bsd_ident::gen_expr() id = " << pos << std::endl;
    e = bsd::bsd_ident::get(pos);
    std::cout << "tok_bsd_ident:: expr = " << e << std::endl;
}


}

#endif


