#ifndef KAS_PARSER_TOKEN_PARSER_H
#define KAS_PARSER_TOKEN_PARSER_H

#include "token_defn.h"

namespace kas::parser
{

// XXX duplicate message

// `token_defn_base` is an abstract base class used to specialize `kas_token` instances
// to convert the parsed "integer" & "raw" types as required.
//
// `token_defn_base` has no instance data & is essentially a way to access a "VTABLE".
// Since `token_defn_base` instances & derived instances are generally created as rvalues
// (ie not permanently allocated), the instance value can't be stored for future refernce.
// Accordingly, the virtual method `get` is defined which returns a reference to a 
// statically created instance of derived type.

// create an actual parser. We need context to complete location tagging
// cribbed from x3::raw_directive
template <typename TOK_DEFN, typename Subject>
struct kas_token_parser : x3::unary_parser<Subject, kas_token_parser<TOK_DEFN, Subject>>
{
    using base_type = x3::unary_parser<Subject, kas_token_parser<TOK_DEFN, Subject>>;
    using attribute_type = kas_token;

    kas_token_parser(Subject const& subject)
        : base_type(subject) {}

    template <typename Iterator, typename Context
                , typename RContext, typename Attribute>
    bool parse(Iterator& first, Iterator const& last
                , Context const& context, RContext const& rcontext, Attribute& attr) const
    {
        // if parsing string, save location, not token `value_t`
        using raw_string = std::basic_string<typename Iterator::value_type>;
        using s_attr = typename x3::traits::attribute_of<Subject, Context>::type;
        using subject_attribute_type = std::conditional_t<
                        std::is_same_v<raw_string, s_attr>, x3::unused_type, s_attr>;

        x3::skip_over(first, last, context);
        Iterator i = first;
#ifdef TOKEN_TRACE
        std::cout << "tok_parser: checking: ";
        print_type_name{TOK_DEFN::name_t::value}.name<s_attr>();
#endif   
        subject_attribute_type value;
        kas_context ctx(context);
        if (this->subject.parse(
                  i, last
                , ctx.token_ctx()
                , rcontext, value))
        {
#ifdef TOKEN_TRACE
        std::cout << " -> matched " << std::string(first, i) << std::endl;
#endif

            // create "token" of `TOK_DEFN` type with parsed location
            auto& handler = x3::get<parser::error_handler_tag>(context); 
            attribute_type tok(TOK_DEFN(), {first, i, &handler});
            
            // store parsed value in token (except for strings)
            if constexpr (!std::is_same_v<decltype(value), x3::unused_type>)
                tok.set(value);
                
            // save token as parsed value
            x3::traits::move_to(tok, attr);

            // consume parsed characters
            first = i;          // update first to just past parsed token

            return true;
        }
        return false;
    }           
};

// create parser when parser is specified as expression
template <typename TOK
        , typename = std::enable_if_t<std::is_base_of_v<token_defn_base, TOK>>>
struct kas_token_x3
{
    template <typename Subject>
    kas_token_parser<TOK, typename x3::extension::as_parser<Subject>::value_type>
    operator[](Subject const& subject) const
    {
        return { x3::as_parser(subject) };
    }
};

// convenience method to define parser for `token_defn_t`
template <typename T> const kas_token_x3<T> token;

}

// extend `x3` to parse tokens

namespace boost::spirit::x3::extension
{

// create parser when `PARSER` is specified as type
template <typename TOK>
struct as_parser<TOK, std::enable_if_t<std::is_base_of_v<kas::parser::token_defn_base, TOK>>>
{
    using Iter      = std::iterator_traits<kas::parser::iterator_type>;
    using string    = std::basic_string<typename Iter::value_type>;
    using kas_token = kas::parser::kas_token;

    // `void` if no parser `type` defined for token
    using parser_t   = typename TOK::parser_t;

    using Derived    = kas::parser::kas_token_parser<TOK, parser_t>;
    //using Derived    = unary_parser<as_parser<string>, kas_token>;
    using type       = Derived const;       // NB: not reference
    using value_type = Derived;             // XXX should this be `kas_token`

    std::enable_if_t<!std::is_void_v<parser_t>, type>
    static call(TOK const& tok)
    {
        print_type_name{"kas_token::as_parser::Derived"}.name<Derived>();
        return x3::as_parser(parser_t());
    }
#if 0
    std::enable_if_t<std::is_void_v<parser_t>, kas_token>
    static call(TOK const& tok)
    {
        return TOK::parser(tok);
    }
#endif
};

}

#endif
