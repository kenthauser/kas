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
            _fixed = obj;
        else if constexpr (std::is_floating_point_v<U>)
            set(obj);
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
#if 0
    // method for non-integral objects
    template <typename T, typename = std::enable_if_t<
                                        !std::is_integral_v<
                                            std::remove_reference_t<T>>>>
    auto set(T&& obj) -> 
        decltype(std::declval<token_defn_base>().set(*this, std::forward<T>(obj)))
    {
        if (!defn_p)
            throw std::logic_error("kas_token::set: set(OBJ) on generic_token");
        defn_p->set(*this, std::forward<T>(obj));
    }
#endif
    // special for arithmetic values
    template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
    std::enable_if_t<sizeof(T) <= sizeof(e_fixed_t)>
    set(T fixed)
    {
        _fixed = fixed;
        data_p = {};
    }

    template <typename T, typename = std::enable_if_t<std::is_floating_point_v<T>>>
    void set(T const& flt)
    {
        if constexpr (!std::is_void_v<expression::e_float_t>)
        {
            //auto& obj = expression::e_float_t::add(flt, *this);
            //set(&obj);
        }
        else
        {
            // XXX create diag
        }
    }


    
    // special for `sym_parser` tokens: parsed result is pointer
    void set(void const *p)
    {
        data_p = p;
    }

    // get `expr()` from token_defn.
    auto& expr()
    {
        // covert string to expr if appropriate
        if (_expr.empty())
            if (defn_p)
                defn_p->gen_expr(_expr, *this);
        
        return _expr;
    }

    // once more, with const...
    auto const& expr() const
    {
        return const_cast<kas_token&>(*this).expr();
    }

    // NB: return `void *` pointer as value (for expression evaluation)
    // mutable pointer required for `core_expr` & `reg_set` expression
    // evaluations. All tokens are transient & `void *` symantics seem natural.
    void *operator()() const
    { 
        // if not `impl` implementation, local pointers can vary
        if (defn_p)
        {
            if (defn_p->is_fixed())
                data_p = &_fixed;
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
    unsigned expr_index() const
    {
        if (defn_p)
            return defn_p->index();
        return {};
    }

    template <typename T>
    T *get_p(T const& = {})
    {
        return expr().get_p<T>();
    }

    template <typename T>
    T const *get_p(T const& = {}) const
    {
        return const_cast<kas_token&>(*this).get_p<T>();
    }

    // test token type: true if token is specified type
    template <typename T>
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

    bool is_missing() const
    { 
        return is_token_type<expression::tok_missing>();
    };


    const char *name() const
    {
        return defn_p ? defn_p->name() : "TOKEN";
    }

    kas_position_tagged const& get_loc() const
    {
        return *this;
    }
    
    void print(std::ostream& os) const;
    
    // should be private
    mutable e_fixed_t      _fixed {};

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

}

#include "token_defn_impl.h"

#endif
