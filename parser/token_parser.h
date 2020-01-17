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
        using raw_string = std::basic_string<typename Iterator::value_type>;
        using s_attr = typename x3::traits::attribute_of<Subject, Context>::type;
        using subject_attribute_type = std::conditional_t<
                        std::is_same_v<raw_string, s_attr>, x3::unused_type, s_attr>;

        subject_attribute_type value;
        x3::skip_over(first, last, context);
        Iterator i = first;

        print_type_name{TOK_DEFN::name_t::value}.name<s_attr>();

        // remove skipper (implicit lexeme)
        auto const& skipper = x3::get<x3::skipper_tag>(context);

        using unused_skipper_type = 
                    x3::unused_skipper<typename std::remove_reference_t<decltype(skipper)>>;
        unused_skipper_type unused_skipper(skipper);

        if (this->subject.parse(
                  i, last
                , x3::make_context<x3::skipper_tag>(unused_skipper, context)
                , rcontext, value))
        {
            // store parsed value in token (except for strings)
            expr_t e;
            if constexpr (!std::is_same_v<decltype(value), x3::unused_type>)
                e = value;
                
            // get the error handler object
            auto& handler = x3::get<parser::error_handler_tag>(context); 
            // create "token" of `TOK_DEFN` type with parsed location
            attribute_type tok(TOK_DEFN(), e, {first, i, &handler});
            
            // save token as parsed value
            x3::traits::move_to(tok, attr);
            
            // consume parsed characters
            first = i;          // update first to just past parsed token

            return true;
        }
        return false;
    }           
};

}
//

namespace boost::spirit::x3::extension
{

template <typename TOK>
struct as_parser<TOK, std::enable_if_t<std::is_base_of_v<kas::parser::token_defn_base, TOK>>>
{
    using parser_t   = typename TOK::parser_t;
    using Derived    = kas::parser::kas_token_parser<TOK, parser_t>;
    using type       = Derived const;       // NB: not reference
    using value_type = Derived;             // XXX should this be `kas_token`

    static type call(TOK const&)
    {
        return x3::as_parser(parser_t());
    }
};

}

namespace kas::parser
{

template <typename TOK
        , typename = std::enable_if_t<std::is_base_of_v<token_defn_base, TOK>>>
struct kas_token_x3
{
    template <typename Subject>
    kas_token_parser<TOK, typename x3::extension::as_parser<Subject>::value_type>
    operator[](Subject const& subject) const
    {
        return { as_parser(subject) };
    }
};

template <typename T> static const kas_token_x3<T> token = {};

}

#endif
