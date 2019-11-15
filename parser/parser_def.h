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
#include "parser_src.h"


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
    operator kas_diag_t const& ()
    {
        if (!diag_p)
            diag_p = &kas_diag_t::warning("Junk following statement, ignored", *this);
        
        std::cout << diag_p->ref() << std::endl;
        return *diag_p;
    }

    operator stmt_error()
    {
        kas_diag_t const& diag = *this;
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

    kas_diag_t const *diag_p {};
};

auto const junk = token<token_junk>[+x3::omit[x3::char_ - stmt_eol]];
auto end_of_line = x3::rule<class _junk> { "eol" } = stmt_eol | junk;

//////////////////////////////////////////////////////////////////////////
//  Parser List Support Methods
//      * combine lists of parsers to single parser
//      * handle empty list case
//////////////////////////////////////////////////////////////////////////

// lambda functions: parse label only & parse to end-of-line
auto const parse_lbl  = [](auto p) { return p; };
auto const parse_eol  = [](auto p) { return p > end_of_line; };
auto const parse_junk = [](auto p) { return p >> x3::omit[junk] >> stmt_eol; };

template <typename F, typename...Ts>
auto make_value_tuple(F&& fn, meta::list<Ts...>&&)
{
    return std::make_tuple(fn(Ts())...);
}
using label_parsers =  all_defns<detail::label_ops_l>;
using stmt_parsers  =  all_defns<detail::stmt_ops_l>;

using XXX_stmt_tuple_t = meta::apply<meta::quote<std::tuple>, stmt_parsers>;
//auto const XXX_stmt_tuple = reduce_tuple(std::bit_or<>(), XXX_stmt_tuple_t());

//auto const stmt_tuple   = make_value_tuple(parse_eol, stmt_parsers()); 
auto const label_tuple  = make_value_tuple(parse_lbl , label_parsers()); 
auto const stmt_tuple   = make_value_tuple(parse_eol , stmt_parsers()); 
auto const junk_tuple   = make_value_tuple(parse_junk, stmt_parsers()); 

auto const statement_def =
#if 1
            reduce_tuple(std::bit_or<>{}, stmt_tuple)
#else
            ( bsd::parser::stmt_comma_x3() > end_of_line )
          | ( bsd::parser::stmt_space_x3() > end_of_line )
          | ( bsd::parser::stmt_equ_x3  () > end_of_line )
          | ( bsd::parser::stmt_org_x3  () > end_of_line )
          //| ( m68k::parser::m68k_stmt_x3  () > end_of_line )
          //| ( z80::parser::z80_stmt_x3  () > end_of_line )
          //| ( arm::parser::arm_stmt_x3  () > end_of_line )
#endif
  //        | reduce_tuple(std::bit_or<>{}, label_tuple)
          | end_of_input
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
        std::cout << "resync_base:: src = \"";
        std::cout << parser_src::escaped_str(std::string{first, last}.substr(0, 60));
        std::cout << "\"..." << std::endl;
        
        // generate & record error message
        base.on_error(first, last, exc, context);

        // resync from error location & forward error
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

        auto message = std::string("\"") + parser_src::escaped_str(std::string{first_unparsed, first}) + "\"";
        std::cout << "unparsed: " << message << std::endl;

        // signal parser to carry on with new "first"
        return x3::error_handler_result::accept;
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
