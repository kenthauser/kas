#ifndef KAS_PARSER_TOKEN_PARSER_H
#define KAS_PARSER_TOKEN_PARSER_H

#include "kas_position.h"
#include "expr/expr.h"

#include <typeinfo>

namespace kas::parser
{

// `token_defn` is an empty virtual (almost abstract) base class used to specialize
// `kas_token` instances to convert the parsed "integer" & "raw" types as required.
//
// `token_defn` has no instance data & is essentially a way to access a "VTABLE".
// Since `token_defn` instances & derived instances are generally created as rvalues
// (ie not permanently allocated), the instance value can't be stored for future refernce.
// Accordingly, the virtual method `get` is defined which returns a reference to a 
// statically created instance of derived type.

struct token_defn
{
    constexpr token_defn() {}

    // get reference to static instance of derived type
    virtual token_defn const& get() const = 0;

    // calculate "expr" from token's "raw" and/or "expr" values
    virtual void gen_expr(expr_t& e, kas_position_tagged const& pos) const = 0;
    
    virtual const char *name() const = 0;
    
    // test if token is particular type
    virtual bool is_token_type(std::type_info const& info) const
    {
        return false;
    }
   
    // convenience method to accept type, not typeid
    template <typename T>
    auto is_token_type(T = {}) const
    {
        return is_token_type(typeid(T));
    }
};

// declared in `parser_types.h`
template <typename NAME, typename VALUE_T, typename PARSER>
struct token_defn_t : token_defn
{
    using name_t   = NAME;
    using value_t  = VALUE_T;
    using parser_T = PARSER;

    token_defn const& get() const override
    {
        // NB: `NAME` is not "literal type" per clang, thus not constexpr
        static const token_defn_t defn;
        return defn;
    }

    const char *name() const override
    {
        return NAME();
    }
    
    // define out-of-line
    void gen_expr(expr_t& e, kas_position_tagged const& pos) const override;

    // test if token is particular type
    bool is_token_type(std::type_info const& info) const override
    {
        return info == typeid(*this);
    }
    
};


struct kas_token : kas_position_tagged
{
    // init from expr/position pair
    kas_token(expr_t e = {}, kas_position_tagged const& pos = {})
        : _expr(std::move(e)), kas_position_tagged(pos) {}

    kas_token(expr_t e, kas_position_tagged const& pos, kas_position_tagged const& pos_end)
        : kas_token(e, pos)
    {
        // update "poisition_tagged" from `begin` to `end`
        if (!handler)
            kas_position_tagged(*this) = pos_end;
        else if (pos_end.handler)
            last = pos_end.last;
    }

    // init used by "parser"
    kas_token(token_defn const& defn, kas_position_tagged const& pos = {})
        : defn_p(&defn.get()), kas_position_tagged{pos} {};


    auto const& expr() const
    {
        // covert string to expr if appropriate
        if (defn_p && _expr.empty())
            defn_p->gen_expr(_expr, *this);
        return _expr;
    }

    // XXX creates ambiguity with drived types
    //operator expr_t const&() const { return expr(); }

    // provide hooks to `expr_t` methods
    auto get_fixed_p() const
    {
        return expr().get_fixed_p();
    }

    template <typename T>
    T const *get_p(T const& = {}) const
    {
        return expr().get_p<T>();
    }

    template <typename T>
    bool is_token_type(T t = {}) const
    {
        return defn_p && defn_p->is_token_type(typeid(T));
    }

    // XXX 
    bool is_missing() const { return false; };

    const char *name() const
    {
        return defn_p ? defn_p->name() : "TOKEN";
    }

    void set_value(expr_t value)
    {
        _expr = value;;
    }

    void print(std::ostream& os) const;

private:
    // create constexpr initializer for `defn_p`
    token_defn const *defn_p {};
    mutable expr_t _expr;
};

// allow streaming of token;
template <typename OS> OS& operator<<(OS& os, kas_token const& tok)
{
    tok.print(os);
    return os;
}

template <typename NAME, typename VALUE_T, typename PARSER>
void token_defn_t<NAME, VALUE_T, PARSER>::gen_expr(expr_t& e, kas_position_tagged const& pos) const
{
    
}


// XXXXXXXXXXX
// create an actual parser. We need context to complete location tagging
// cribbed from x3::raw_directive
template <typename TOK_DEFN, typename Subject>
struct X_kas_token_parser : x3::unary_parser<Subject, X_kas_token_parser<TOK_DEFN, Subject>>
{
    using base_type = x3::unary_parser<Subject, X_kas_token_parser<TOK_DEFN, Subject>>;
    using attribute_type = kas_token;

    X_kas_token_parser(Subject const& subject)
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

        using unused_skipper_type = 
                    x3::unused_skipper<typename std::remove_reference_t<decltype(skipper)>>;
        unused_skipper_type unused_skipper(skipper);

        if (this->subject.parse(
                  i, last
                , x3::make_context<x3::skipper_tag>(unused_skipper, context)
                , rcontext, value))
        {
            auto& handler = x3::get<parser::error_handler_tag>(context).get();

            // create "token" of `TOK_DEFN` type with parsed location
            attribute_type token(TOK_DEFN(), {first, i, &handler});
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
                token.set_value(value);
                
            // consume parsed characters
            first = i;          // update first to just past parsed token

            // save token as parsed value
            x3::traits::move_to(token, attr);
            return true;
        }
        return false;
    }           
};

template <typename TOK, typename = std::enable_if_t<std::is_base_of_v<token_defn, TOK>>>
struct X_kas_token_x3
{
    template <typename Subject>
    X_kas_token_parser<TOK, typename x3::extension::as_parser<Subject>::value_type>
    operator[](Subject const& subject) const
    {
        return { as_parser(subject) };
    }

    // XXX need to return default if [] not used

};

template <typename T> static const X_kas_token_x3<T> X_token = {};
// XXXXXXXXXXX

#if 1
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
#endif
}

#endif
