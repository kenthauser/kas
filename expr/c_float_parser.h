#ifndef KAS_EXPR_C_FLOAT_PARSER_H
#define KAS_EXPR_C_FLOAT_PARSER_H

// parse floating point numbers into container
// using c++11/14/17 languange formats

#include "literal_types.h"
#include <stdexcept>
#include <cassert>

namespace kas::expression::parser
{
    auto decode_flt = [](auto& ctx)
    {
        _val(ctx)  = kas_float::add(_attr(ctx));
    };

    auto parse_flt = [](auto&& parser, auto&& decode)
    {
        return  parser[decode];
    };

    e_float_parser<kas_float> c_float_p = "floating point parser (c-rules)";

    auto const c_float_p_def = lexeme[
            parse_flt(strict_float_p<long double>(), decode_flt)
        ];

    BOOST_SPIRIT_DEFINE(c_float_p)
}


#endif
