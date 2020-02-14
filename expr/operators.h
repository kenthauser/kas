#ifndef KAS_EXPR_OPERATORS_H
#define KAS_EXPR_OPERATORS_H

#include "expr.h"
#include "expr_types.h"
#include "expr_op_types.h"
#include "kas/defn_utils.h"
#include "kas/kas_string.h"
//#include "parser/token_defn.h"
#include "parser/token_parser.h"

// type to instantiate definitions and perform string parsing
#include "parser/sym_parser.h"

///////////////////////////////////////////////////////////////////////////
// Declare binary & unary operators
///////////////////////////////////////////////////////////////////////////

namespace kas::expression::detail
{
// NB: in detail namespace, all types are ``meta``
using namespace meta;
using namespace precedence;

// no STL template for: uplus, shift_left, shift_right. Define replacements
struct op_uplus {
    template <typename T>
    constexpr decltype(auto) operator()(T&& arg) const
        { return std::forward<T>(arg); }
};
struct op_shift_left {
    template <typename T, typename U>
    constexpr auto operator()(T&& t, U&& u) const
            -> decltype(std::forward<T>(t) << std::forward<U>(u))
        { return std::forward<T>(t) << std::forward<U>(u); }
};
struct op_shift_right {
    template <typename T, typename U>
    constexpr auto operator()(T&& t, U&& u) const
            -> decltype(std::forward<T>(t) >> std::forward<U>(u))
        { return std::forward<T>(t) >> std::forward<U>(u); }
};

//
// declare meta lists of binary & unary operations and types
//
#define STR KAS_STRING

// Binary op fields: PRIORITY, IS_DIVIDE, NAME, OP, ALIASES...
template <> struct bin_ops_v<defn_expr> : list<
      list<int_<PRI_PLUS> , bool_<false>, STR("+") , std::plus<>>
    , list<int_<PRI_MINUS>, bool_<false>, STR("-") , std::minus<>>
    , list<int_<PRI_MULT> , bool_<false>, STR("*") , std::multiplies<>>
    , list<int_<PRI_DIV>  , bool_<true> , STR("/") , std::divides<>>
    , list<int_<PRI_MOD>  , bool_<true> , STR("%") , std::modulus<>>
    , list<int_<PRI_LSHFT>, bool_<false>, STR("<<"), op_shift_left>
    , list<int_<PRI_RSHFT>, bool_<false>, STR(">>"), op_shift_right>
    , list<int_<PRI_AND>  , bool_<false>, STR("&") , std::bit_and<>, STR(".and.")>
    , list<int_<PRI_XOR>  , bool_<false>, STR("^") , std::bit_xor<>, STR(".xor.")>
    , list<int_<PRI_OR>   , bool_<false>, STR("|") , std::bit_or<> , STR(".or.")>
    > {};

// Unary op fields: NAME, OP, ALIASES...
// NB: all priority `PRI_PFX` / `PRI_SFX` 
template <> struct pfx_ops_v<defn_expr> : list<
      list<STR("-"), std::negate<>>
    , list<STR("+"), op_uplus>
    , list<STR("~"), std::bit_not<>, STR(".not.")>
    > {};

#undef STR

///////////////////////////////////////////////////////////////////////////
// Combine all definitions to create operation type lists
//
// Add boilerplate PREFIXES: ARITY for all; PRECEDENCE and DIVIDE_OP for pfx/sfx
///////////////////////////////////////////////////////////////////////////

template <template<typename> class T, typename...Ts>
using gen_types = transform<
          all_defns<T>
        , bind_front<quote<concat>, list<Ts...>>
        >;

using bin_types = gen_types<bin_ops_v, int_<2>>;
using pfx_types = gen_types<pfx_ops_v, int_<1>, int_<PRI_PFX>, bool_<false>>;
using sfx_types = gen_types<sfx_ops_v, int_<1>, int_<PRI_SFX>, bool_<false>>;

// since bin_type/pfx_type/sfx_types share same type, need to share NAMES
// use `sym_parser_t` meta_functions to retrieve needed types
namespace
{
    // perform calculations in temporary namespace
    using kas::parser::detail::get_types_one_item;

    using NAME_LIST = typename expr_op_defn::NAME_LIST;

    template <typename DEFNS, typename XTRA = list<>>
    using get_expr_op_names = _t<apply<get_types_one_item<DEFNS>
                                     , list<void, NAME_LIST, void, XTRA>>>;
    using NAMES     = list<get_expr_op_names<pfx_types,
                              get_expr_op_names<bin_types>>>;
}

// create parsers from lists
static const auto bin_ops = parser::sym_parser_t<expr_op_defn, bin_types, void, NAMES>();
static const auto pfx_ops = parser::sym_parser_t<expr_op_defn, pfx_types, void, NAMES>();
static const auto sfx_ops = parser::sym_parser_t<expr_op_defn, sfx_types, void, NAMES>();
}

//
// declare expr_op parsers in proper namespace
//
namespace kas::expression::parser
{

// declare parsed "tokens"
using TOK_OPER_BIN = token_defn_t<KAS_STRING("OP_BIN"), expr_op>;
using TOK_OPER_PFX = token_defn_t<KAS_STRING("OP_PFX"), expr_op>;
using TOK_OPER_SFX = token_defn_t<KAS_STRING("OP_SFX"), expr_op>;

// declare "parsers"
// NB: x3_oper is "raw" parser, not requiring "x3::lexeme[(&) >> !x3::graphic]"
auto const bin_op_x3 = x3::no_case[detail::bin_ops.x3_raw()];
auto const pfx_op_x3 = x3::no_case[detail::pfx_ops.x3_raw()];
auto const sfx_op_x3 = x3::no_case[detail::sfx_ops.x3_raw()];

// associate parsers & tokens
auto const tok_bin_op = parser::token<TOK_OPER_BIN>[bin_op_x3];
auto const tok_pfx_op = parser::token<TOK_OPER_PFX>[pfx_op_x3];
auto const tok_sfx_op = parser::token<TOK_OPER_SFX>[sfx_op_x3];
}
#endif

