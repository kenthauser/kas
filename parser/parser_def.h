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

using stmt_comment   = meta::_t<detail::stmt_comment_str<>>;
using stmt_separator = meta::_t<detail::stmt_separator_str<>>;

//////////////////////////////////////////////////////////////////////////
//  Parser Support Methods
//      * end-of-line
//      * end-of-input
//      * junk (non-matching input)
//////////////////////////////////////////////////////////////////////////

// parse to comment, separator, or end-of-line
auto const stmt_eol = x3::rule<class _> {"stmt_eol"} =
      x3::lit(stmt_comment()()) >> 
      x3::omit[*(x3::char_ - x3::eol)] >> 
      -x3::eol
    | x3::lit(stmt_separator()())
    | x3::eol
    ;

// absorb characters until EOL
x3::rule<class _> skip_eol {"skip_eol"};
auto const skip_eol_def = stmt_eol | x3::graph >> skip_eol;

using tok_invalid = token_defn_t<KAS_STRING("INVALID")>;
auto const invalid = token<tok_invalid>[x3::omit[+x3::graph]];

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
//x3::rule<class _stmt    ,  stmt_t> const statement   = "statement";

x3::rule<class _tag_stmt, stmt_t> const tagged_stmt = "tagged stmt";
stmt_x3 stmt { "stmt" };


// insn is statment (after skipping blank or commented lines)
// NB: parse "statement" separately because X3 sees `variant` base
// class in `stmt_t` & slices away `kas_position_tagged` base class.
// Solution: have "statment" perform variant operaion & tag afterwords
auto const stmt_def  = *stmt_eol > tagged_stmt;

// require statements to extend to end-of-line (or separator)
// not required for labels
x3::rule<struct insn_junk,  typename stmt_t::base_t> parse_insn { "parse_insn" };
auto const parse_insn_def = combine_parsers(stmt_parsers()) > stmt_eol;
BOOST_SPIRIT_DEFINE(parse_insn);

auto const parse_invalid = x3::rule<class _, detail::stmt_diag> { "parse_invalid"} 
                  = invalid >> x3::omit[-skip_eol];

auto const statement_def =
           parse_insn
         | combine_parsers(label_parsers())
         | x3::omit[x3::eoi] >> x3::attr(stmt_eoi())
         | parse_invalid
         ;

// tag stmt after fully parsed
auto const tagged_stmt_def = statement;

BOOST_SPIRIT_DEFINE(stmt, statement, tagged_stmt, skip_eol)

///////////////////////////////////////////////////////////////////////////
// Annotation and Error handling
///////////////////////////////////////////////////////////////////////////

// parser to find point to restart scan after error
auto const resync = *(x3::char_ - stmt_eol) >> stmt_eol;

// handle where unparsed data after insn, but before EOL
struct insn_junk
{
    template <typename Iterator, typename Exception, typename Context>
    auto on_error(Iterator& first , Iterator const& last
                , Exception const& exc, Context const& context)
    {
        // examine context
        auto& obj       = x3::get<error_diag_tag>(context);
        auto& e_handler = x3::get<error_handler_tag>(context);
        
        // save parser positions
        auto before = first;
        auto junk   = exc.where();
        bool no_eol = false;
        
        // find end-of-line: first -> next parse location
        if (!parse(first, last, resync))
        {
            first  = last;
            no_eol = true;
        }

        kas_position_tagged junk_loc { junk, first, &e_handler };
        //kas_position_tagged junk_loc { junk, junk, &e_handler };
        std::cout << "insn_junk::on_error: " << junk_loc.where() << std::endl;
       
        // process "junk" following statement
        bool ignore_junk = false;
        std::string msg = "Junk following statement";

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

}

#endif
