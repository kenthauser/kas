#ifndef KAS_PARSER_TOKEN_DEFN_IMPL_H
#define KAS_PARSER_TOKEN_DEFN_IMPL_H

#include "kas_token.h"

#include <typeinfo>

namespace kas::parser
{

#if 0
auto token_defn_base::
    set(kas_token& tok, expression::detail::float_value<> flt) -> void
{
    //auto& obj = expression::e_float_t::add(flt, tok);
    //tok.set(&obj);
}

// assign value from kas_object
template <typename T, typename>
void token_defn_base::set(kas_token& tok, T& obj)
{
    tok.set(&obj);
}
#endif

// define `token_defn_t` methods which access `kas_token` methods
// or require `e_fixed_t` or `expr_t` definition
template <typename NAME, typename VALUE_T, typename PARSER>
bool token_defn_t<NAME, VALUE_T, PARSER>::
            is_fixed() const 
{
    return std::is_integral_v<value_t> && (sizeof(e_fixed_t) <= sizeof(value_t));
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
    return expr_t::index<value_t>();
}

// convert "tokens" to "expressions"
template <typename NAME, typename VALUE_T, typename PARSER>
void token_defn_t<NAME, VALUE_T, PARSER>::
            gen_expr(expr_t& e, kas_token const& tok) const
{
    // get pointer to value
    auto p = (*this)(&tok);

    // wrapped types need to be location tagged
    if constexpr (meta::in<expr_t::unwrapped, value_t>::value)
        e = { *p, tok };
    // XXX not correct, but...
    //else if constexpr (std::is_constructible<expr_t, VALUE_T>)
    else if constexpr(meta::in<expr_t::plain, value_t>::value)
        e = *p;
}

template <typename NAME, typename VALUE_T, typename PARSER>
auto token_defn_t<NAME, VALUE_T, PARSER>::
            operator()(kas_token const *p) const -> value_t const *
{
    // require a token_p
    if (!p)
        p = token_p;
    if (!p)
        throw std::logic_error{"token_defn_t::operator(): null token"};

    // require matching token type
    if (!p->is_token_type(*this))
        return {};

    void *data_p;
    if (is_fixed())
        data_p = &p->_fixed;     // XXX not friend...
    else
        data_p = (*p)();
    return static_cast<value_t const *>(data_p);
}

namespace detail
{
    template <typename PARSER, typename = void>
    struct get_parser_value : meta::id<void> {};

    template <typename PARSER>
    struct get_parser_value<PARSER, std::void_t<typename PARSER::attribute_type>>
            : meta::id<typename PARSER::attribute_type> {};
}


template <typename NAME, typename VALUE_T, typename PARSER>
void const *token_defn_t<NAME, VALUE_T, PARSER>::
            gen_data_p(kas_token const& tok
                     , std::type_info const& info
                     , void const *obj
                     ) const
{
    // if parser specified as part of type, retrieve parsed value
    using PARSED_VALUE_T = typename detail::get_parser_value<PARSER>::type; 

    std::cout << "token_defn_t::gen_data_p()" << std::endl;
    print_type_name{"token_defn_t::gen_data_p()::VALUE_T"}.name<value_t>();
    print_type_name{"token_defn_t::gen_data_p()::PARSED_VALUE"}.name<PARSED_VALUE_T>();
    if constexpr (std::is_integral_v<value_t> && sizeof(value_t) <= sizeof(e_fixed_t))
        return &tok._fixed;

    else if (typeid(VALUE_T) == info)
        return obj;

    else if constexpr (!std::is_void_v<PARSED_VALUE_T> &&
                        std::is_base_of_v<core::kas_object_tag, VALUE_T>
                        && !std::is_same_v<VALUE_T, e_string_t>)

    {
        if (typeid(PARSED_VALUE_T) == info)
        {
            auto p = static_cast<PARSED_VALUE_T const *>(obj);
            return &VALUE_T::add(*p);
        }

        else
        {
            std::cout << "token_defn_t: info mismatch: info = " << info.name();
            std::cout << ", PARSED_VALUE = " << typeid(PARSED_VALUE_T).name();
            std::cout << std::endl;
        }
    }

    return nullptr;
}
}

#endif
