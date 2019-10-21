#ifndef KAS_PARSER_TOKEN_PARSER_H
#define KAS_PARSER_TOKEN_PARSER_H

#include "kas_position.h"
#include "expr/expr.h"

#include <typeinfo>

namespace kas::parser
{

struct token_defn
{
    // default construct: just a token
    constexpr token_defn() = default;

    // named token type construct: save info
    template <typename T>
    token_defn(T) : defn_index{typeid(T)}
    {
        static_assert(std::is_base_of_v<token_defn, T>,
                    "token_defn::ctor: not initialized from derived type");

        constexpr static T defn;
        defn_p = &defn;
    }

    virtual expr_t gen_expr(kas_token const&) const
    {
        return {};
    }

    virtual const char *name() const
    {
        return "TOKEN";
    }

    template <typename T>
    bool is_token_v(T = {}) const
    {
        return defn_index == typeid(T);
    }

    // test if default token type
    // XXX should this be renamed: is_unnamed_token_v()
    bool is_token_v() const
    {
        return !defn_p;
    }


protected:
    token_defn const *defn_p    {};
    std::type_index  defn_index {typeid(void)};
};

template <typename VALUE_T = void>
struct token_defn_t : token_defn
{
    using value_t = VALUE_T;

    token_defn_t() : token_defn(token_defn_t()) {}
};


struct kas_token : kas_position_tagged
{
    // init from expr/position pair
    kas_token(expr_t e = {}, kas_position_tagged const& pos = {})
        : expr(std::move(e)), kas_position_tagged(pos) {}

    // init from "parser"
    kas_token(token_defn& defn) : defn_p(&defn) {};

    operator expr_t() const
    {
        // covert string to expr if appropriate
        if (defn_p && expr.empty())
            expr = defn_p->gen_expr(*this);
        return expr;
    }

private:
    token_defn const *defn_p {};
    mutable expr_t expr;
};

// allow streaming of token;
template <typename OS> OS& operator<<(OS&, kas_token const&);

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

        typedef x3::unused_skipper<typename std::remove_reference_t<decltype(skipper)>>
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
            token.set_first  (first);
            token.set_last   (i);
            token.set_handler(&x3::get<parser::error_handler_tag>(context).get());
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
