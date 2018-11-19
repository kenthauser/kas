#ifndef PARSER_PARSER_CONFIG_H
#define PARSER_PARSER_CONFIG_H 

#include <boost/spirit/home/x3.hpp>
#include <meta/meta.hpp>

namespace kas::parser
{
namespace x3 = boost::spirit::x3;

namespace detail {

    template <typename T, T V>
    struct parser_defn_t
    {
        using type = parser_defn_t;
        static constexpr std::decay_t<T> value = V;
        decltype(auto) operator()() const { return V; }
    };

    template <typename = void> struct label_ops_l : meta::list<> {};
    template <typename = void> struct stmt_ops_l  : meta::list<> {};
}


// Our Iterator Type
using iterator_type = std::string::const_iterator;
using char_type     = typename std::iterator_traits<iterator_type>::value_type;

// blank_type matches spaces or tabs, but not newlines
using skipper_t = x3::ascii::blank_type;

// Declare our Error Handlers  (see "parser/error_handler.h")

// KAS extension for multiple files
template <typename Iter> struct error_handler;

// handler stored in context
using error_handler_type = error_handler<iterator_type>;

// tag used to get our error handler from the context
struct error_handler_tag;

// XXX move this to parser_obj
// The Phrase Parse Context
using phrase_context_type = x3::phrase_parse_context<skipper_t>::type;

// Combined Error Handler and Phrase Parse Context
using error_context_type = x3::context<
                        kas::parser::error_handler_tag
                      , std::reference_wrapper<error_handler_type> //const
              , phrase_context_type
          >;

using context_type = error_context_type;

struct kas_context
{
    using x3_type = context_type;

    template <typename Context>
    kas_context(Context& ctx) :
        error_handler_ref(x3::get<error_handler_type>(ctx))
        {}

    kas_context(error_handler_type& e) : error_handler_ref(e) {}

    x3_type x3() 
    {
        // XXX run down reference issues.
        auto skipper_ctx = x3::make_context<x3::skipper_tag>(as_parser(skipper));
        auto error_ctx   = x3::make_context<error_handler_tag>(error_handler_ref, skipper_ctx);
        return error_ctx;
    }

    auto& error_handler()
    {
        return error_handler_ref.get();
    }
    
private:
    static constexpr skipper_t skipper {};
    std::reference_wrapper<error_handler_type> error_handler_ref;
};


// forward declare `position_tagged` template & type
template <typename Iter> struct kas_position_tagged_t;
using kas_position_tagged = kas_position_tagged_t<iterator_type>;

}

#endif
