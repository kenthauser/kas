#ifndef KAS_PARSER_PARSER_DEF_H
#define KAS_PARSER_PARSER_DEF_H

#ifdef BOOST_SPIRIT_X3_DEBUG
#include <iostream>
#endif

#include "parser.h"                 // parser public interface
#include "parser_variant.h"         // variant of all stmt types
#include "error_handler_base.h"
#include "annotate_on_success.hpp"

//#include "machine_parsers.h"
//#include "utility/make_type_over.h"

#include <boost/spirit/home/x3.hpp>
#include <utility>

namespace kas::parser
{
namespace x3 = boost::spirit::x3;

// instantiate comment and seperator parsers...
auto stmt_comment   = detail::stmt_comment_p  <
        typename detail::stmt_comment_str<>::type
        >();
auto stmt_separator = detail::stmt_separator_p<
        typename detail::stmt_separator_str<>::type
        >();

//////////////////////////////////////////////////////////////////////////
//  Parser Support Methods
//      * end-of-line
//      * end-of-input
//      * junk (non-matching input)
//////////////////////////////////////////////////////////////////////////

// parse to comment, separator, or end-of-line
auto const stmt_eol = x3::rule<class _> {"stmt_eol"} =
      stmt_comment > *(x3::omit[x3::char_] - x3::eol) > (x3::eol | x3::eoi)
    | stmt_separator
    | x3::eol
    ;

// parser to recognize end-of-input
struct _tag_eoi : annotate_on_success {};
auto const end_of_input = x3::rule<_tag_eoi, stmt_eoi> { "eoi" } = x3::eoi;

// parse invalid content to end-of-line or comment
struct token_junk: kas_token
{
    operator kas_diag const& ()
    {
        if (!diag_p)
            diag_p = &kas_diag::warning("Junk following statement, ignored", *this);
        
        std::cout << diag_p->ref() << std::endl;
        return *diag_p;
    }

    operator stmt_error()
    {
        kas_diag const& diag = *this;
        std::cout << "token_junk: stmt_error: " << diag.ref() << std::endl; 
        return diag;
    }

    ~token_junk()
    {
        std::cout << "token_junk destroyed";

        if (diag_p)
            std::cout << ": " << diag_p->ref();
        std::cout << std::endl;

        if (this->handler)
            std::cout << "token_junk: parsed data: " << *this << std::endl;

    }

    kas_diag const *diag_p {};
};

auto const junk = token<token_junk>[+x3::omit[x3::char_ - stmt_eol]];
auto end_of_line = x3::rule<class _junk> { "eol" } = stmt_eol | junk;

//////////////////////////////////////////////////////////////////////////
//  Parser List Support Methods
//      * combine lists of parsers to single parser
//      * handle empty list case
//////////////////////////////////////////////////////////////////////////
#if 0
namespace detail
{
    using rule_t = x3::rule<class _, stmt_t>;
    
    // general recursion case
    template <typename RT, typename T, typename...Ts>
    rule_t make_parser_impl(RT parser, meta::list<T, Ts...>)
    {
        // recurse if more parsers
        if constexpr (sizeof...(Ts) != 0)
            return make_parser_impl(parser | x3::as_parser(T()), meta::list<Ts...>());
    
        // exit recursion
        return parser | x3::as_parser(T());
    }

    // handle list of single or multiple parsers
    template <typename T, typename...Ts>
    rule_t make_parser(meta::list<T, Ts...>)
    {
        // if multiple, recurse.
        if constexpr (sizeof...(Ts) != 0)
            return make_parser_impl(x3::as_parser(T()), meta::list<Ts...>());

        // if single element, just first
        return x3::as_parser(T());
    }

    // handle empty list: don't match
    auto make_parser(meta::list<>)
    {
        return x3::eps(false);
    }

}

// generate parser from stmts & lists
template <template<typename=void> class LIST>
auto make_parser_from_list()
{
    return detail::make_parser(all_defns<LIST>());
}

// get meta list of parsers from config vectors<>
auto const label_parsers = make_parser_from_list<detail::label_ops_l>();
auto const stmt_parsers  = make_parser_from_list<detail::stmt_ops_l >();

#else
// lambda functions: parse label only & parse to end-of-line
auto const parse_lbl  = [](auto&& p) { return p; };
auto const parse_eol  = [](auto&& p) { return p >> stmt_eol; };
auto const parse_junk = [](auto&& p) { return p >> x3::omit[junk] >> stmt_eol; };

template <typename F, typename...Ts>
auto make_value_tuple(F&& fn, meta::list<Ts...>&&)
{
    return std::make_tuple(fn(Ts())...);
}
using label_parsers =  all_defns<detail::label_ops_l>;
using stmt_parsers  =  all_defns<detail::stmt_ops_l>;

using XXX_stmt_tuple_t = meta::apply<meta::quote<std::tuple>, stmt_parsers>;
auto const XXX_stmt_tuple = reduce_tuple(std::bit_or<>(), XXX_stmt_tuple_t());

//auto const stmt_tuple   = make_value_tuple(parse_eol, stmt_parsers()); 
auto const label_tuple  = make_value_tuple(parse_lbl , label_parsers()); 
auto const stmt_tuple   = make_value_tuple(parse_eol , stmt_parsers()); 
auto const junk_tuple   = make_value_tuple(parse_junk, stmt_parsers()); 
#endif

auto const statement_def =
#if 1
            reduce_tuple(std::bit_or<>{}, stmt_tuple)
#else
            ( bsd::parser::stmt_comma_x3() > end_of_line )
          | ( bsd::parser::stmt_space_x3() > end_of_line )
          | ( bsd::parser::stmt_equ_x3  () > end_of_line )
          | ( bsd::parser::stmt_org_x3  () > end_of_line )
          | ( z80::parser::z80_stmt_x3  () > end_of_line )
#endif
          | reduce_tuple(std::bit_or<>{}, label_tuple)
          | end_of_input
         // | reduce_tuple(std::bit_or<>{}, junk_tuple)
          | junk
          ;


x3::rule<class _stmt, stmt_t> const statement = "statement";
stmt_x3 stmt { "stmt" };

// insn is statment (after skipping blank or commented lines)
auto const stmt_def  = *stmt_eol > statement;

BOOST_SPIRIT_DEFINE(stmt, statement)

///////////////////////////////////////////////////////////////////////////
// Annotation and Error handling
///////////////////////////////////////////////////////////////////////////

// annotation is performed `per-parser`. only error-handling is here.

// parser to find point to restart scan after error
auto const resync = *(x3::char_ - stmt_eol) > stmt_eol;

struct resync_base
{

    template <typename Iterator, typename Exception, typename Context>
    auto on_error(Iterator& first , Iterator const& last
                , Exception const& exc, Context const& context)
    {
        // generate & record error message
        base.on_error(first, last, exc, context);

        // resync from error location & forward error.
        first = exc.where();
    
        auto first_unparsed = first;

        try
        {
            parse(first, last, resync);
        }
        catch (x3::expectation_failure<Iterator> const& err)
        {
            // XXX need to mark as warning so as not to step on error...
            // XXX send error to std::out...
            auto& error_handler = x3::get<error_handler_tag>(context).get();
            error_handler(std::cout, err.where(), "Warning: No trailing newline");
            first = last;
        }

    auto message = "\"" + std::string(first_unparsed, first) + "\"";
    std::cout << "unparsed: " << message << std::endl;

        // return error from parser
        return x3::error_handler_result::fail;
    }

private:
    // parser base error handler
    kas::parser::error_handler_base base;
};

// XXX undef of statement screws up parser 
//struct _tag_stmt : annotate_on_success {};
struct _stmt : resync_base {};
struct _junk : resync_base {};

// interface to statement parser

}

#endif
