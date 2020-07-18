#ifndef KAS_PARSER_COMBINE_PARSERS_H
#define KAS_PARSER_COMBINE_PARSERS_H

#include <tuple>

namespace kas::parser
{

template <typename...Ts, typename FN_T = std::nullptr_t>
constexpr auto combine_parsers(std::tuple<Ts...> const& args, FN_T fn = {})
{
    // if no parsers, return parser that doesn't match anything
    if constexpr (sizeof...(Ts) == 0)
    {
        return x3::eps(false);
    }
    
    // if `fn` specified, preprocess parsers with `fn`
    else if constexpr (!std::is_null_pointer_v<FN_T>)
    {
        auto combine_fn = [&fn](auto&&...p) { return (x3::as_parser(fn(p)) | ...); };
        return std::apply(combine_fn, args);
    }

    // otherwise just combine all parsers
    else 
    {
        auto combine_fn = [](auto&&...p) { return (x3::as_parser(p) | ...); };
        return std::apply(combine_fn, args);
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
