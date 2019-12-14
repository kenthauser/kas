#ifndef KAS_PARSER_COMBINE_PARSERS_H
#define KAS_PARSER_COMBINE_PARSERS_H

#include <tuple>

namespace kas::parser
{

template <typename...Ts>
auto combine_parsers(std::tuple<Ts...>const& args)
{
    // if no parsers, return parser that doesn't match anything
    if constexpr (sizeof...(Ts) == 0)
    {
        return x3::eps(false);
    }
    
    // otherwise combine all parsers
    else
    {
        auto combine_fn = [](auto&&...p) { return (x3::as_parser(p) | ...); };
        return std::apply(combine_fn, args);
    }
}

// accept a `meta::list` as argument. 
template <typename...Ts>
auto combine_parsers(meta::list<Ts...>)
{
    return combine_parsers({Ts()...});
}

}


#endif
