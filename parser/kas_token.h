#ifndef KAS_PARSER_KAS_TOKEN_H
#define KAS_PARSER_KAS_TOKEN_H

#include "parser_types.h"
#include "token_defn.h"
#include "expr/expr.h"

#include <typeinfo>

namespace kas::parser
{

//#define KAS_TOKEN_TRACE

struct kas_token : kas_position_tagged
{
    // init from expr/position pair
    kas_token(expr_t e = {}, kas_position_tagged const& pos = {})
        : _expr(std::move(e)), kas_position_tagged(pos)
    {
#ifdef KAS_TOKEN_TRACE
        std::cout << "TOK: CTOR: e=" << e << " src=\"" << std::string(pos) << "\"" << std::endl;
        std::cout << "TOK: alloc: " << std::hex << this << std::endl;
#endif
    }

    kas_token(expr_t e, kas_position_tagged const& pos, kas_position_tagged const& pos_end)
        : kas_token(e, pos)
    {
        // update "poisition_tagged" from `begin` to `end`
        if (!handler)
            kas_position_tagged(*this) = pos_end;
        else if (pos_end.handler)
            last = pos_end.last;

#ifdef KAS_TOKEN_TRACE
        std::cout << "TOK: CTOR: new_src=\"" << std::string(*this) << "\"" << std::endl;
        std::cout << "TOK: alloc: " << std::hex << this << std::endl;
#endif
    }

    // init used by "parser"
    kas_token(token_defn_base const& defn, expr_t e = {}, kas_position_tagged const& pos = {})
        : defn_p(&defn.get()), _expr(e), kas_position_tagged(pos)
    {
#ifdef KAS_TOKEN_TRACE
        std::cout << "TOK: CTOR: name=" << defn_p->name();
        std::cout << ", e=" << _expr;
        std::cout << ", src=\"" << std::string(pos) << "\"" << std::endl;
        std::cout << "TOK: alloc: " << std::hex << this << std::endl;
#endif
    };

#if 0
    kas_token(kas_token const& tok) : kas_position_tagged(tok)
    {
#ifdef KAS_TOKEN_TRACE
        std::cout << "TOK: arg " << std::hex << &tok << std::endl;
        std::cout << "TOK: CTOR(const&): name=" << tok.name();
        std::cout << " e=" << tok._expr << " src=\"" << std::string(tok) << "\"" << std::endl;
        std::cout << "TOK: alloc: " << std::hex << this << std::endl;
        if (tok.handler && *tok.first == '1' && tok._expr.empty())
            throw *this;
#endif
        defn_p = tok.defn_p;
        _expr  = tok._expr;
        std::cout << "TOK: result = ";
        print(std::cout);
        std::cout << std::endl;
    }

    kas_token& operator= (kas_token const& tok)
    {
#ifdef KAS_TOKEN_TRACE
        std::cout << "TOK: arg " << std::hex << &tok << std::endl;
        std::cout << "TOK: operator=(const&): name=" << tok.name();
        std::cout << " e=" << tok._expr << " src=\"" << std::string(tok) << "\"" << std::endl;
        
        if (tok.handler && *tok.first == '1' && tok._expr.empty())
            throw *this;
#endif
        static_cast<kas_position_tagged&>(*this) = tok;
        defn_p = tok.defn_p;
        _expr  = tok._expr;
        std::cout << "TOK: result " << std::hex << this << std::endl;
        std::cout << "TOK: result = ";
        print(std::cout);
        std::cout << std::endl;
        return *this;
    }

    ~kas_token()
    {
        std::cout << "TOK: dtor " << std::hex << this << std::endl;
    }
#endif

    // get `expr()` from token_defn.
    // used for "intermediate" expressions. Don't generate 'parser::loc'
    auto& raw_expr()
    {
        // covert string to expr if appropriate
        if (defn_p && _expr.empty())
            defn_p->gen_expr(_expr, *this);
        return _expr;
    }

    // gernerate location tagged expression
    auto& expr()
    {
        auto& e = raw_expr();

        // set `loc` for wrapped types
        e.set_loc(*this);
        return e;
    }

    auto const& expr() const
    {
        return const_cast<kas_token&>(*this).expr();
    }

    // XXX creates ambiguity with drived types
    //operator expr_t const&() const { return expr(); }

    // provide hooks to `expr_t` methods
    auto get_fixed_p() const
    {
        // fixed values are not location tagged.
        //return raw_expr().get_fixed_p();
        return expr().get_fixed_p();
    }

    template <typename T>
    T const *get_p(T const& = {}) const
    {
        return expr().get_p<T>();
    }

    // test for token type: `void` tests for generic `kas_token`
    template <typename T = void>
    token_defn_base const * is_token_type() const
    {
#if 0
        // XXX move to _impl
        // return pointer to "generic" defn iff `void`
        if constexpr (std::is_void_v<T>) 
        {
            static token_defn_t<KAS_STRING("EXPR")> undef;
            if (!defn_p)
                return &undef;
        }
#endif
        // return pointer to "defn" if type matches
        if (defn_p)
            return defn_p->is_token_type(typeid(T));

        return {};
    }

    // convenience method: pass instance of type
    template <typename T>
    auto is_token_type(T) const
    {
        return is_token_type<T>();
    }

    // XXX 
    bool is_missing() const { return false; };

    const char *name() const
    {
        return defn_p ? defn_p->name() : "TOKEN";
    }
#if 0
    void set_value(expr_t value)
    {
        _expr = value;;
    }
#endif
    
    void print(std::ostream& os) const;

private:
    token_defn_base const *defn_p {};
    expr_t                 _expr;
};

// allow streaming of token;
template <typename OS> OS& operator<<(OS& os, kas_token const& tok)
{
    tok.print(os);
    return os;
}



#if 1

// XXX

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
#endif // store parsed value in token (except for strings)
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

// create parser iff attribute type is `kas_token`
template <typename TOK
        , typename = std::enable_if_t<std::is_base_of_v<kas_token, TOK>>
        >
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
