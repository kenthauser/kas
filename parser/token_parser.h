#ifndef KAS_PARSER_TOKEN_PARSER_H
#define KAS_PARSER_TOKEN_PARSER_H

#include "kas_position.h"

namespace kas::parser
{

// create an actual parser. We need context to complete location tagging
// cribbed from x3::raw_directive
template <typename TOK, typename Subject>
struct kas_token_parser : x3::unary_parser<Subject, kas_token_parser<TOK, Subject>>
{
    using base_type = x3::unary_parser<Subject, kas_token_parser<TOK, Subject>>;
    using attribute_type = TOK;

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

        // remove skipper (implicit lexeme)
        auto const& skipper = x3::get<x3::skipper_tag>(context);

        typedef x3::unused_skipper<
            typename std::remove_reference<decltype(skipper)>::type>
        unused_skipper_type;
        unused_skipper_type unused_skipper(skipper);

        if (this->subject.parse(
                  i, last
                , x3::make_context<x3::skipper_tag>(unused_skipper, context)
                , rcontext, value))
        {
            TOK token;
#if 0
            print_type_name("token_parser::token_type").name<TOK>(std::cout); 
            //print_type_name("token_parser::subject::s_attr")
            //        .name<s_attr>(std::cout);
            
            //print_type_name("token_parser::subject_type").name<subject_attribute_type>(std::cout); 
            print_type_name("token_parser::attribute ").name<Attribute>(std::cout);
            std::cout << "token_parser: matched: " << std::string(first, i);
            std::cout << std::endl;
#endif
            // store parsed value in token (except for strings)
            if constexpr (!std::is_same_v<decltype(value), x3::unused_type>)
                token.value = value;
            
            // save location in token
            token.first   = first;
            token.last    = i;
            token.handler = &x3::get<parser::error_handler_tag>(context).get();
            first = i;          // update first to just past parsed token

            // save token as parsed value
            x3::traits::move_to(token, attr);
            return true;
        }
        return false;
    }           
};

template <typename TOK, typename = std::enable_if_t<std::is_base_of_v<kas::parser::kas_position_tagged, TOK>>>
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
