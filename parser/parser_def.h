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

x3::rule<class _stmt    , typename stmt_t::base_t> const statement   = "statement";
x3::rule<class _tag_stmt, stmt_t> const tagged_stmt = "tagged stmt";
stmt_x3 stmt { "stmt" };

// insn is statment (after skipping blank or commented lines)
// NB: parse "statement" separately because X3 sees `variant` base
// class in `stmt_t` & slices away `kas_position_tagged` base class.
// Solution: have "statment" perform variant operaion & tag afterwords
auto const stmt_def  = *stmt_eol > tagged_stmt;

// require statements to extend to end-of-line (or separator)
// not required for labels
auto const statement_def =
            combine_parsers(stmt_parsers(), parse_eol)
          | combine_parsers(label_parsers())
          ;

auto const tagged_stmt_def = statement;


BOOST_SPIRIT_DEFINE(stmt, statement, tagged_stmt)

///////////////////////////////////////////////////////////////////////////
// Annotation and Error handling
///////////////////////////////////////////////////////////////////////////

// annotation is performed `per-parser`. only error-handling is here.

// parser to find point to restart scan after error
auto const resync = *(x3::char_ - stmt_eol) >> stmt_eol;

struct stmt_invalid
{
    template <typename Iterator, typename Exception, typename Context>
    auto on_error(Iterator& first , Iterator const& last
                , Exception const& exc, Context const& context)
    {
        std::cout << "stmt_invalid::on_error" << std::endl;
        // skip error line
        if (!parse(first, last, resync))
            first = last;
        
        // return failure to parse instruction
        return x3::error_handler_result::fail;
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
        // save parser positions
        auto before = first;
        auto junk = exc.where();
        
        std::cout << "stmt_junk::on_error" << std::string(first, junk) << std::endl;
       
        // find end-of-line: first -> next parse location
        if (!parse(first, last, resync))
            first = last;

        // process "junk" following statement
        bool ignore_junk = false;
        std::string msg = "Junk following statement";

        auto& obj       = x3::get<error_diag_tag>(context);
        auto& e_handler = x3::get<error_handler_tag>(context);

        kas_position_tagged junk_loc { junk, first, &e_handler };
        
        if (ignore_junk)
        {
            // generate warning message about junk & re-parse
            kas_diag_t::warning(msg + ", ignored", junk_loc);
            obj.parse(before, junk);

            // apparently X3 steps on "re-parse" return value if just
            // return "accept". so come in thru side door.
            throw first;
            
            // front door, which doesn't seem to work.
            return x3::error_handler_result::accept;
        }
        
        obj.err_idx = kas_diag_t::error(msg, junk_loc).ref();
        return x3::error_handler_result::accept;
    }

private:
    // parser base error handler
    kas::parser::error_handler_base base;
};

struct _tag_stmt : annotate_on_success {};
struct _stmt     : stmt_junk    {};

}

#endif
