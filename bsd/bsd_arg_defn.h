#ifndef KAS_BSD_BSD_ARG_DEFN_H
#define KAS_BSD_BSD_ARG_DEFN_H

#include "parser/parser_types.h"
#include "bsd_symbol.h"
#include <ostream>

namespace kas::bsd
{

// declare the "token" types (parsed, but not evaluated)
using kas::parser::kas_token;

//#define TRACE_TOKEN

struct token_ident : kas_token
{
    operator core::symbol_ref() const
    {
        auto ref   = bsd_ident::get(*this); 
        ref.set_loc(*this);
#ifdef TRACE_TOKEN
        std::cout << "token_ident: " << std::string(*this) << " -> ";
        ref.print(std::cout);
        std::cout << " local_loc = " << static_cast<::kas::parser::kas_loc const&>(*this).get();
        std::cout << std::endl;
#endif
        return ref;
    }
};

struct token_local_ident : kas_token
{
    uint32_t value;
    operator core::symbol_ref() const
    {
        auto ref = bsd_local_ident::get(*this, value);
        ref.set_loc(*this);
#ifdef TRACE_TOKEN
        std::cout << "token_local_ident: " << std::string(*this) << " -> ";
        ref.print(std::cout);
        std::cout << " local_loc = " << static_cast<::kas::parser::kas_loc const&>(*this).get();
        std::cout << std::endl;
#endif
        return ref;
    }
};

struct token_numeric_ident : kas_token
{
    operator core::symbol_ref() const
    {
        auto n = *first - '0';
        auto p = std::next(first);
        bool dir = (p != last) && (*p == 'f' || *p == 'F'); 
        auto ref = bsd_numeric_ident::get(*this, n, dir);
        ref.set_loc(*this);
#ifdef TRACE_TOKEN
        std::cout << "token_numeric_ident: " << std::string(*this) << " -> ";
        ref.print(std::cout);
        std::cout << std::endl;
#endif
        return ref;
    }
};

struct token_dot      : kas_token 
{
    operator core::addr_ref() const
    { 
        auto ref = core::core_addr_t::get_dot().ref(*this);
#ifdef TRACE_TOKEN
        std::cout << "token_dot: -> ";
        ref.print(std::cout);
        std::cout << std::endl;
#endif
        return ref;
    }
};

struct token_at_ident : kas_token {};
struct token_at_num   : kas_token { unsigned value{}; };
struct token_missing  : kas_token {};
 

using token_types = meta::list<
              token_ident
            , token_dot
            , token_at_ident
            , token_at_num
            , token_missing
            >;

// XXX use `NAME` member-types & write some MPL for this       
static constexpr const char *bsd_token_names[] =
            { "IDENT", "DOT", "@IDENT", "@NUM", "MISSING" };

// XXX write some MPL for this
static constexpr bool token_has_value[] =
            { false, false, false, true, false };


// declare container of args
using bsd_args = std::vector<struct bsd_arg>;

// all `bsd_arg` instances are location tagged (for errors) 
struct bsd_arg : kas_token
{
    bsd_arg() = default;    // required by x3

    template <typename T>
    static auto constexpr get_idx()
    {
        return 1 + meta::find_index<token_types, T>::value;
    }

    template <typename T>
    static bool constexpr in()
    {
        return get_idx<T>() != 0;
    }

    // arg is either an expression or a "token"
    // init from expression
    bsd_arg(expr_t&& e) : expr(std::move(e)) {}

    // init from bsd token parsed
    template <typename T, typename = std::enable_if_t<in<T>()>>
    bsd_arg(T const& t) : token_idx(get_idx<T>()), kas_token(t)
    {
        if constexpr (kas::parser::has_value<T>()) {
            expr = t.value;
        }
    }

    bool is_token() const
    {
        return token_idx;
    }

    bool has_value() const
    {
        return !is_token() || token_has_value[token_idx-1];
    }


    // used in opcode::validate_min_max()
    bool is_missing() const
    {
        constexpr auto missing_idx = get_idx<token_missing>();
        return token_idx == missing_idx;
    }

    operator expr_t&&()
    {
        if (is_missing())
            expr = ::kas::parser::kas_diag_t::error("Missing argument", *this);
        else if (!has_value())
            expr = ::kas::parser::kas_diag_t::error("Invalid expression", *this);
        return std::move(expr);
    }

    template <typename T, typename = std::enable_if_t<in<T>()>>
    kas_token const *get_p() const
    {
        constexpr auto idx = get_idx<T>();
        if (token_idx == idx)
            return this;
        return {};
    }

    template <typename T, typename = std::enable_if_t<!in<T>()>>
    auto get_p() const
    {
        return expr.get_p<T>();
    }

    expression::e_fixed_t const *get_fixed_p() const
    {
        if (!has_value())
            return {};
        return expr.get_fixed_p();
    }

private:
    friend std::ostream& operator<<(std::ostream&, bsd_arg const&);
    
    expr_t    expr;
    uint8_t   token_idx {};

    // verify index of non-specified token is zero
    void _() { static_assert(bsd_arg::get_idx<void>() == 0); }
};
    
// ostream "tokens" as bsd_arg
template <typename T, typename = std::enable_if_t<bsd_arg::in<T>()>>
inline std::ostream& operator<<(std::ostream& os, T const& t)
{
    return os << bsd_arg(t);
}

}

#endif

