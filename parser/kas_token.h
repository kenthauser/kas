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
    // create "default" token from (optional) expr/position pair
    kas_token(expr_t e = {}, kas_position_tagged const& pos = {})
        : _expr(std::move(e)), kas_position_tagged(pos)
    {}

    // init used by token_parser: create non-default token
    kas_token(token_defn_base const& defn
            , expr_t e = {}
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

        if (auto p = e.get_p<core::symbol_ref>())
        {
            std::cout << "kas_token::expr() -> " << *p << std::endl;
            p->set_loc(*this);
        }
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
    expr_t                 _expr;
};

// allow streaming of token;
template <typename OS> OS& operator<<(OS& os, kas_token const& tok)
{
    tok.print(os);
    return os;
}

}

#endif
