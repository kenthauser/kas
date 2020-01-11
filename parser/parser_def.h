#ifndef KAS_PARSER_PARSER_DEF_H
#define KAS_PARSER_PARSER_DEF_H

#ifdef BOOST_SPIRIT_X3_DEBUG
#include <iostream>
#endif

#include "parser.h"                 // parser public interface
#include "parser_variant.h"         // variant of all stmt types
#include "combine_parsers.h"
#include "error_handler_base.h"
#include "annotate_on_success.hpp"

#include "parser_src.h"
#include "token_parser.h"


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
      stmt_comment >> x3::omit[*(x3::char_ - x3::eol)] >> -x3::eol
    | stmt_separator
    | x3::eol
    ;

//////////////////////////////////////////////////////////////////////////
//  Parser List Support Methods
//      * combine lists of parsers to single parser
//      * handle empty list case
//////////////////////////////////////////////////////////////////////////

// lambda function: parse to end-of-line
auto const parse_eol  = [](auto p) { return p > stmt_eol; };

using label_parsers =  all_defns<detail::label_ops_l>;
using stmt_parsers  =  all_defns<detail::stmt_ops_l>;

x3::rule<class _stmt, stmt_t> const statement = "statement";
stmt_x3 stmt { "stmt" };

// insn is statment (after skipping blank or commented lines)
auto const stmt_def  = *stmt_eol > statement;

// require statements to extend to end-of-line (or separator)
// not required for labels
auto const statement_def =
            combine_parsers(stmt_parsers(), parse_eol)
          | combine_parsers(label_parsers())
          ;


BOOST_SPIRIT_DEFINE(stmt, statement)

///////////////////////////////////////////////////////////////////////////
// Annotation and Error handling
///////////////////////////////////////////////////////////////////////////

// annotation is performed `per-parser`. only error-handling is here.

// parser to find point to restart scan after error
auto const resync = *(x3::char_ - stmt_eol) >> -stmt_eol;

struct stmt_invalid
{
    template <typename Iterator, typename Exception, typename Context>
    auto on_error(Iterator& first , Iterator const& last
                , Exception const& exc, Context const& context)
    {
        // save first "error" iter
        auto first_unparsed = first;

        // skip to next line
        parse(first, last, resync);
        
        // NB: `first` now points at end-of-instruction
        // generate & record error message
        base.on_error(first_unparsed, first, exc, context);

        // continue and parse next instruction
        // "first" updated to next instruction -- re-parse from there
        return x3::error_handler_result::retry;
    }

private:
    // parser base error handler
    kas::parser::error_handler_base base;
};

struct stmt_junk
{
    template <typename Iterator, typename Exception, typename Context>
    auto on_error(Iterator& first , Iterator const& last
                , Exception const& exc, Context const& context)
    {
        std::cout << "stmt_junk:: src = \"";
        std::cout << parser_src::escaped_str(std::string{first, last}.substr(0, 60));
        std::cout << "\"..." << std::endl;
        
        print_type_name{"context"}.name<Context>();

        auto& error_diag = x3::get<error_diag_tag>(context).get();
        print_type_name{"error_diag"}(error_diag);


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
struct _tag_stmt : stmt_invalid {};
struct _junk : stmt_junk    {};
// interface to statement parser

}

#endif
