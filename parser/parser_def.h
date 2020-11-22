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
      ( x3::lit(stmt_comment()()) >> 
        x3::omit[*(x3::char_ - x3::eol)] >> 
        -x3::eol
      )
    | x3::lit(stmt_separator()())
    | x3::eol
    ;

// absorb characters until EOL
x3::rule<class _> skip_eol {"skip_eol"};
auto const skip_eol_def = stmt_eol | x3::graph >> skip_eol;

// parser to find point to restart scan after error
auto const resync = *(x3::char_ - stmt_eol);


//////////////////////////////////////////////////////////////////////////
//  Parser List Support Methods
//      * combine lists of parsers to single parser
//      * handle empty list case
//////////////////////////////////////////////////////////////////////////

// lambda function: parse to end-of-line
auto const parse_eol  = [](auto p) { return p > stmt_eol; };

using label_parsers =  all_defns<detail::label_ops_l>;
using stmt_parsers  =  all_defns<detail::stmt_ops_l>;

x3::rule<class insn_junk, stmt_t> const statement   = "statement";

x3::rule<class _tag_stmt, stmt_t> const tagged_stmt = "tagged stmt";
stmt_x3 stmt { "stmt" };

x3::rule<class _insn_err,  stmt_t> parse_insn { "parse_insn" };
auto const parse_insn_def = combine_parsers(stmt_parsers());
BOOST_SPIRIT_DEFINE(parse_insn);


// insn is statment (after skipping blank or commented lines)
// NB: parse "statement" separately because X3 sees `variant` base
// class in `stmt_t` & slices away `kas_position_tagged` base class.
// Solution: have "statment" perform variant operaion & tag afterwords
auto const stmt_def  = *stmt_eol > tagged_stmt;

// Parse an "invalid" instruction (invalid opcode, not mismatched args)
// 
// Parse is a little tricky because want to tag "opcode", not whole insn.
// 1. Create an "invalid" token & attach parser which will
//    match any unmatched graphic sequence.
// 2. Create a rule which matches "invalid" & absorbs to end-of-line.
// 3. Create `lambda` to convert "token" (which is position tagged)
//    to diagnostic. Then create a "stmt_t" with diagnostic & 
//    is tagged as only the "invalid" token.


// 1. create token & define parser
using tok_invalid = token_defn_t<KAS_STRING("INVALID")>;
auto const invalid = token<tok_invalid>[+x3::graph];

// 3. create `lambda` to generate stmt from token
auto gen_diag = [](auto& ctx)
    {
        // get "invalid" token
        auto& tok = x3::_attr(ctx);
        
        // create `stmt_t` from diagnostic (with token `loc`)
        x3::_val(ctx) = kas_diag_t::error("Invalid opcode", tok);
    };

// 2. create parse rule to match "junk" and skip to eol
auto const parse_invalid = x3::rule<class _, stmt_t> {"parse_invalid"} 
                  = invalid[gen_diag] > x3::omit[resync];

// Parse actual statemnt: here whitespace lines have been absorbed.
auto const statement_def =
           parse_insn        > stmt_eol
         | combine_parsers(label_parsers())
         | x3::omit[x3::eoi] >> x3::attr(stmt_eoi()())
         | parse_invalid
         ;

// tag stmt after fully parsed
auto const tagged_stmt_def = statement;

BOOST_SPIRIT_DEFINE(stmt)
BOOST_SPIRIT_DEFINE(statement)
BOOST_SPIRIT_DEFINE(tagged_stmt)
BOOST_SPIRIT_DEFINE(skip_eol)

///////////////////////////////////////////////////////////////////////////
// Annotation and Error handling
///////////////////////////////////////////////////////////////////////////

struct _insn_err : annotate_on_success
{
    template <typename iterator, typename exception, typename Context>
    auto on_error(iterator& first , iterator const& last
                , exception const& exc, Context const& context)
    {
        // examine context
        auto& obj       = x3::get<error_diag_tag>(context);
        auto& e_handler = x3::get<error_handler_tag>(context);
        
        // save parser positions
        auto before = first;
        auto where  = exc.where();
        bool no_eol = false;

        print_type_name{"_insn_err::obj"}.name<decltype(obj)>();
        std::cout << "_insn_err::on_error: exc = " << exc.what() << std::endl;
        std::cout << "_insn_err::on_error: which = " << exc.which() << std::endl;
        kas_position_tagged parsed = { first, where, &e_handler };
        std::cout << "_insn_err::parsed: " << parsed.where() << std::endl;

        // find end-of-line: first -> next parse location
        if (!parse(first, last, resync))
        {
            first  = last;
            no_eol = true;
        }

        std::string msg = "Expected " + exc.which();
        kas_position_tagged err_loc = { where, where, &e_handler };

        obj.err_idx = kas_diag_t::error(msg, err_loc).ref();

        // record whole line as "matching"
        //e_handler.tag(x3::_val(context), before, first);
        
        // accept error as result
        return x3::error_handler_result::accept;
    }
};

// handle where unparsed data after insn, but before EOL
struct insn_junk
{
    template <typename iterator, typename exception, typename Context>
    auto on_error(iterator& first , iterator const& last
                , exception const& exc, Context const& context)
    {
        // examine context
        auto& obj       = x3::get<error_diag_tag>(context);
        auto& e_handler = x3::get<error_handler_tag>(context);
        
        // save parser positions
        auto before = first;
        auto junk   = exc.where();
        bool no_eol = false;

        print_type_name{"insn_junk::obj"}.name<decltype(obj)>();
        std::cout << "insn_junk::on_error: exc = " << exc.what() << std::endl;
        std::cout << "insn_junk::on_error: which = " << exc.which() << std::endl;
        kas_position_tagged parsed = { first, junk, &e_handler };
        std::cout << "insn_junk::parsed: " << parsed.where() << std::endl;

        // find end-of-line: first -> next parse location
        if (!parse(first, last, resync))
        {
            first  = last;
            no_eol = true;
        }

        // diagnostic: from { exc.where() -> resync }
        kas_position_tagged junk_loc { junk, first, &e_handler };
        std::cout << "insn_junk::on_error: " << junk_loc.where() << std::endl;
       
        // process "junk" following statement
        bool ignore_junk = false;
        ignore_junk = true;
        std::string msg = "Expected " + exc.which();

        if (ignore_junk)
        {
            // put "junk" in a warning message
            kas_diag_t::warning("Junk following statement, ignored", junk_loc).ref();

            // "junk" consumed by moving `last`. Just accept
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
