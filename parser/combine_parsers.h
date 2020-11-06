#ifndef KAS_PARSER_COMBINE_PARSERS_H
#define KAS_PARSER_COMBINE_PARSERS_H

#include <tuple>

namespace kas::parser
{

// accept a `std::tuple` naming parsers to be combined via `or`
template <typename...Ts, typename FN_T = std::nullptr_t>
constexpr auto combine_parsers(std::tuple<Ts...> const& args, FN_T fn = {})
{
    // if no parsers specified, return a parser that doesn't match anything
    if constexpr (sizeof...(Ts) == 0)
    {
        return x3::eps(false);
    }
    
    // if `fn` specified, preprocess parsers with `fn`
    else if constexpr (!std::is_null_pointer_v<FN_T>)
    {
        return (x3::as_parser(fn(Ts())) | ... );
    }

    // otherwise just combine all parsers
    else 
    {
        return (x3::as_parser(Ts()) | ... );
    }
}

// accept a `meta::list` as argument. 
template <typename...Ts, typename FN_T = std::nullptr_t>
constexpr auto combine_parsers(meta::list<Ts...>, FN_T fn = {})
{
    return combine_parsers<Ts...>(std::make_tuple(Ts()...), fn);
}

}


#endif
