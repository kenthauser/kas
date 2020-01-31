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
    //kas_token() = default;

    // create "default" token from (optional) expr/position pair
    kas_token(expr_t e, kas_position_tagged const& pos = {})
        : _expr(std::move(e)), kas_position_tagged(pos)
    {}

    // init used by token_parser: create non-default token
    kas_token(token_defn_base const& defn
            , expr_t e
            , kas_position_tagged const& pos = {}
            )
        : defn_p(&defn.get()), _expr(std::move(e)), kas_position_tagged(pos)
    {}

    // init from expression and pair of position tags (used by expr evaluation)
    kas_token(expr_t e
            , kas_position_tagged const& pos
            , kas_position_tagged const& pos_end
            )
        : kas_token(e, pos)
    {
        // update "poisition_tagged" from `begin` to `end`
        if (!handler)
            kas_position_tagged(*this) = pos_end;
        else if (pos_end.handler)
            last = pos_end.last;
    }
    
    // create "default" token from position pair
    kas_token(kas_position_tagged const& pos = {})
        : kas_position_tagged(pos)
    {}

    // init used by token_parser: create non-default token
    kas_token(token_defn_base const& defn
            , kas_position_tagged const& pos = {}
            )
        : defn_p(&defn.get()), kas_position_tagged(pos)
    {}

    // init from pair of position tags (used by expr evaluation)
    kas_token(kas_position_tagged const& pos
            , kas_position_tagged const& pos_end
            )
        : kas_token(pos)
    {
        // update "poisition_tagged" from `begin` to `end`
        if (!handler)
            kas_position_tagged(*this) = pos_end;
        else if (pos_end.handler)
            last = pos_end.last;
    }

    // set data or expression depending on argument
    void set(void const* p)
    {
        data_p = p;
        _expr  = {};
    }
    
    template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
    std::enable_if_t<sizeof(T) <= sizeof(e_fixed_t)>
    set(T fixed)
    {
        _expr = fixed;
        data_p = _expr.get_fixed_p();
    }
    
    // get `expr()` from token_defn.
    // used for "intermediate" expressions. Don't generate 'parser::loc'
    auto& raw_expr()
    {
        // set data pointer first
        if (defn_p && !data_p)
            data_p = defn_p->gen_data_p(*this);
        // covert string to expr if appropriate
        if (defn_p && _expr.empty())
            defn_p->gen_expr(_expr, *this);
        return _expr;
    }

    // gernerate location tagged expression
    auto& expr()
    {
        auto& e = raw_expr();

        if (auto p = e.get_loc_p())
            std::cout << "kas_token::expr (pre): " << p->where() << std::endl;
        // set `loc` for wrapped types
        e.set_loc(*this);
        if (auto p = e.get_loc_p())
            std::cout << "kas_token::expr (set): " << p->where() << std::endl;
        return e;
    }

    auto const& expr() const
    {
        return const_cast<kas_token&>(*this).expr();
    }

    // return void * pointer to value (for expression evaluation)
    auto operator()() const { return data_p; }

    // XXX creates ambiguity with drived types
    //operator expr_t const&() const { return expr(); }

    // provide hooks to `expr_t` methods
    auto get_fixed_p() const
    {
        // fixed values are not location tagged.
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
    
    void print(std::ostream& os) const;

private:
    token_defn_base const *defn_p {};
    void            const *data_p {};
    expr_t                 _expr;
};

// allow streaming of token;
template <typename OS> OS& operator<<(OS& os, kas_token const& tok)
{
    tok.print(os);
    return os;
}

// define `token_defn_t` methods which access `kas_token` methods
// or require `expr_t` definition
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
    // integral types already stored in expr
    if constexpr (std::is_integral_v<VALUE_T> &&
                  sizeof(VALUE_T) <= sizeof(e_fixed_t))
    {
        /* expr already holds value */
    }

    // wrapped types need to be location tagged
    else if constexpr (meta::in<expr_t::unwrapped, VALUE_T>())
    {
        e = static_cast<VALUE_T const *>(tok())->ref(tok);
    }

    // variant types can directly initialize expression
    else if constexpr (meta::in<expr_t::variant_types, VALUE_T>())
    {
        e = *static_cast<VALUE_T const *>(tok());
    }

#if 1
    else
    {
        throw std::logic_error{"token_defn_t::gen_expr(): not an expression"};
    }
#endif
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


#if 1
template <typename NAME, typename VALUE_T, typename PARSER>
VALUE_T const *token_defn_t<NAME, VALUE_T, PARSER>::operator()() const
{
    if (token_p && token_p->is_token_type(*this))
        return static_cast<VALUE_T const *>((*token_p)());
    std::cout << "token_defn_t::operator(): no match" << std::endl;
    return {};
}
#endif
}

#endif
