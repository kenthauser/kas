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
    
    virtual const char *name() const
    {
        return "TOKEN";
    }
//#define XXX_VIRT_IS_TOKEN
#ifdef  XXX_VIRT_IS_TOKEN   
    // test if token is particular type
    virtual token_defn const *is_token_type(std::type_info const& info) const
    {
        return {};
    }
#else
    // test if token is particular type
    token_defn const *is_token_type(std::type_info const& info) const
    {
        // NB: warning generated by `typeid(get())`
        auto& t = get();
        return info == typeid(t) ? this : nullptr;
    }
#endif

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
    using value_t  = VALUE_T;       // NB: default if `void`
    using parser_T = PARSER;        // NB: default is `void`

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

#ifdef  XXX_VIRT_IS_TOKEN   
    // test if token is particular type
    token_defn const* is_token_type(std::type_info const& info) const override
    {
        return (info == typeid(*this)) ? this : nullptr;
    }
#endif
};

#define KAS_TOKEN_TRACE

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
    kas_token(token_defn const& defn, expr_t e = {}, kas_position_tagged const& pos = {})
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

    auto& raw_expr()
    {
        // covert string to expr if appropriate
  //      if (defn_p && _expr.empty())
  //          defn_p->gen_expr(_expr, *this);
        return _expr;
    }

    auto& expr()
    {
        auto& e = raw_expr();
        //e.set_loc(*this);
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
#if 0
    void set_value(expr_t value)
    {
        _expr = value;;
    }
#endif
    
    void print(std::ostream& os) const;

private:
    // create constexpr initializer for `defn_p`
    token_defn const *defn_p {};
    expr_t            _expr;
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
            
            // store parsed value in token (except for strings)
            expr_t e;
            if constexpr (!std::is_same_v<decltype(value), x3::unused_type>)
                e = value;
                
            attribute_type token(TOK_DEFN(), e, {first, i, &handler});
#if 0
            print_type_name("token_parser::token_type").name<TOK>(std::cout); 
            //print_type_name("token_parser::subject::s_attr")
            //        .name<s_attr>(std::cout);
            
            //print_type_name("token_parser::subject_type").name<subject_attribute_type>(std::cout); 
            print_type_name("token_parser::attribute ").name<Attribute>(std::cout);
            std::cout << "token_parser: matched: " << std::string(first, i);
            std::cout << std::endl;
#endif            
            
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
