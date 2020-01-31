#ifndef KAS_BSD_BSD_ARG_IMPL_H
#define KAS_BSD_BSD_ARG_IMPL_H

#include "bsd_arg_defn.h"
#include "bsd_symbol.h"

namespace kas::parser
{

// NB: all of this is in namespace `kas::parser`
#if 0
template <> void bsd::tok_bsd_ident
        ::gen_expr(expr_t& e, kas_token const& pos) const
{
    std::cout << "tok_bsd_ident::gen_expr() id = " << pos << std::endl;
    e = bsd::bsd_ident::get(pos);
    std::cout << "tok_bsd_ident:: expr = " << e << std::endl;
}
#else
template <> void const *bsd::tok_bsd_ident
        ::gen_data_p(kas_token const& tok) const
{
    std::cout << "tok_bsd_ident::gen_data_p() token = " << tok << std::endl;
    auto& sym = bsd::bsd_ident::get(tok);
    std::cout << "tok_bsd_ident:: sym = " << expr_t(sym) << std::endl;
    return &sym;
}
#endif

}

#endif


