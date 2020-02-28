#ifndef KAS_PARSER_KAS_TOKEN_H
#define KAS_PARSER_KAS_TOKEN_H

#include "parser_types.h"
#include "token_defn.h"
#include "expr/expr.h"

#include <typeinfo>

namespace kas::parser
{

struct kas_token : kas_position_tagged
{
    // X3 requires default constructible types
    kas_token() = default;
    
    // ctor used by token_parser: create non-default token
    kas_token(token_defn_base const& defn
            , kas_position_tagged const& pos = {}
            )
        : defn_p(&defn.get()), kas_position_tagged(pos) {}

    // ctor used by expression evaluation
    // metafunction `expression::token_t` maps value types -> token types
    template <typename T
            , typename U   = std::remove_reference_t<T>
            , typename TOK = meta::_t<expression::token_t<U>>
            , typename     = std::enable_if_t<!std::is_void_v<TOK>>
            >
    kas_token(T&& obj) : defn_p{&TOK().get()}
    {
        if constexpr (std::is_integral_v<U>)
            _expr = obj;
        else
            data_p = &obj;

        // diagnostics have location already tagged
        if constexpr (std::is_same_v<U, kas_diag_t>)
            tag(obj.loc());
    }

    kas_token(expr_t& e);
    
    void tag(kas_position_tagged const& pos
           , kas_position_tagged const& pos_end = {})
    {
        static_cast<kas_position_tagged&>(*this) = {pos, pos_end};
    }

    // set data or expression depending on argument
    void set(void const *p)
    {
        data_p = p;
        _expr  = {};
    }

    template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
    std::enable_if_t<sizeof(T) <= sizeof(e_fixed_t)>
    set(T fixed)
    {
        _expr = fixed;
    }
#if 0
    // allow tagged values as tokens
    template <typename T>
    set(core::ref_loc_t<T> const& ref)
    {

    }
#endif
    // get `expr()` from token_defn.
    auto const& expr() const
    {
        // covert string to expr if appropriate
        if (defn_p && _expr.empty())
            defn_p->gen_expr(_expr, *this);
        
        return _expr;
    }

    // allow conversion from `token` to `expr`
    //operator expr_t const&() const { return expr(); }

    // NB: return `void *` pointer as value (for expression evaluation)
    // mutable pointer required for `core_expr` & `reg_set` expression
    // evaluations. All tokens are transient & `void *` symantics seem natural.
    void *operator()() const
    { 
        // if not `impl` implementation, local pointers can vary
        if (defn_p)
        {
            if (defn_p->is_fixed())
                data_p = _expr.get_fixed_p();
            else if (!data_p)
                data_p = defn_p->gen_data_p(*this);
        }
        return const_cast<void *>(data_p);
    }

    // provide hooks to `expr_t` methods
    auto get_fixed_p() const
    {
        // fixed values are not location tagged.
        return expr().get_fixed_p();
    }

    // get expression variant index (+1)
    unsigned index() const
    {
        if (defn_p)
            return defn_p->index();
        return {};
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
#if 1
    // XXX 
    bool is_missing() const { return false; };
#endif
    const char *name() const
    {
        return defn_p ? defn_p->name() : "TOKEN";
    }
    
    void print(std::ostream& os) const;

private:
    template <typename T, typename HAS_LOC>
    void init(T const&, HAS_LOC);

    // `_expr` & `data_p` hold side effects from text->value conversion
    // thus mark mutable
    mutable expr_t         _expr; 
    mutable void const    *data_p {};   // allow conversion from string to object
    token_defn_base const *defn_p {};
};

// allow streaming of token;
template <typename OS> OS& operator<<(OS& os, kas_token const& tok)
{
    tok.print(os);
    return os;
}


/////////////////////////////////////////////////////////////////////////////////////

// define `token_defn_t` methods which access `kas_token` methods
// or require `e_fixed_t` or `expr_t` definition

template <typename NAME, typename VALUE_T, typename PARSER>
bool token_defn_t<NAME, VALUE_T, PARSER>::
            is_fixed() const 
{
    return std::is_integral_v<VALUE_T> && (sizeof(e_fixed_t) <= sizeof(VALUE_T));
}
   
template <typename NAME, typename VALUE_T, typename PARSER>
token_defn_t<NAME, VALUE_T, PARSER>::operator bool() const
{
    return token_p && token_p->is_token_type<token_defn_t>();
}

template <typename NAME, typename VALUE_T, typename PARSER>
unsigned token_defn_t<NAME, VALUE_T, PARSER>::
            index() const 
{
    return expr_t::index<VALUE_T>();
}
    
// convert "tokens" to "expressions"
template <typename NAME, typename VALUE_T, typename PARSER>
void token_defn_t<NAME, VALUE_T, PARSER>::
            gen_expr(expr_t& e, kas_token const& tok) const
{
    // get pointer to value
    auto p = (*this)(&tok);

    // wrapped types need to be location tagged
    if constexpr (meta::in<expr_t::unwrapped, VALUE_T>::value)
        e = p->ref(tok);
#if 0
    // XXX expr_t allows `expr_op` to `fail` ctor even with following
    else if constexpr (std::is_constructible_v<expr_t, decltype(*p)>)
        e = *p;
#endif
}

template <typename NAME, typename VALUE_T, typename PARSER>
VALUE_T const *token_defn_t<NAME, VALUE_T, PARSER>::
            operator()(kas_token const *p) const
{
    // require a token_p
    if (!p)
        p = token_p;
    if (!p)
        throw std::logic_error{"token_defn_t::operator(): null token"};

    // require matching token type
    if (!p->is_token_type(*this))
        return {};
    return static_cast<VALUE_T const *>((*p)());
}

template <typename NAME, typename VALUE_T, typename PARSER>
void const *token_defn_t<NAME, VALUE_T, PARSER>::
            gen_data_p(kas_token const& tok) const
{
    if constexpr (std::is_integral_v<VALUE_T> && sizeof(VALUE_T) <= sizeof(e_fixed_t))
        return tok.get_fixed_p();
    else
        return nullptr;
}
}

#endif
