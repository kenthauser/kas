#ifndef KAS_PARSER_KAS_TOKEN_IMPL_H
#define KAS_PARSER_KAS_TOKEN_IMPL_H

#include "kas_token.h"
#include <boost/spirit/home/x3/support/utility/lambda_visitor.hpp>

namespace kas::parser
{

kas_token::kas_token(expr_t& e)
{
#if 0
    auto visitor = [](auto const& value)
    {
        using value_t = std::remove_reference_t<decltype(value)>;
        using TOK = meta::_t<expression::token_t<value_t>;
    }

    if (auto p = e.get_fixed_p())
    {
        using FIXED_TOK = meta::_t<expression<expression::token_t<e_fixed_t>>>;
        defn_p = &TOK().get();
        _expr  = *p;     // manually set value
    }
    else 
    {
        // if location tagged, retrieve pointer
        auto loc_p = e.get_loc_p();
    }
#endif
}


    
}

#endif
