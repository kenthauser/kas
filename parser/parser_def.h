#ifndef KAS_PARSER_PARSER_DEF_H
#define KAS_PARSER_PARSER_DEF_H

#ifdef BOOST_SPIRIT_X3_DEBUG
#include <iostream>
#endif

#include "parser.h"                 // parser public interface
#include "error_handler_base.h"
#include "annotate_on_success.hpp"

#include "machine_parsers.h"
#include "utility/make_type_over.h"

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
//  Assembler Instruction Parser Definition
//////////////////////////////////////////////////////////////////////////

auto const stmt_eol = x3::rule<class _> {"stmt_eol"} =
      stmt_comment > *(x3::omit[x3::char_] - x3::eol) > (x3::eol | x3::eoi)
    | stmt_separator
    | x3::eol
    ;

// insn defns
x3::rule<class _stmt,     stmt_t> const stmt = "stmt";
x3::rule<class statement, stmt_t> const statement = "statement";

// get meta list of parsers from config vectors<>
using label_parsers =  all_defns<detail::label_ops_l>;
using stmt_parsers  =  all_defns<detail::stmt_ops_l>;

// lambda functions: parse label only & parse to end-of-line
auto const parse_eol = [](auto&& p) { return p()() > stmt_eol; };
auto const parse_lbl = [](auto&& p) { return p()(); };

using stmt_tuple_t = meta::apply<meta::quote<std::tuple>, stmt_parsers>;
auto const stmt_tuple = reduce_tuple(std::bit_or<>(), stmt_tuple_t());

using label_tuple_t = meta::apply<meta::quote<std::tuple>, label_parsers>;
auto const label_tuple = reduce_tuple(std::bit_or<>(), label_tuple_t());

struct junk_token : kas_token
{
    operator kas_error_t() const
    {
        return kas_diag::error("Invalid opcode").ref(*this);
    }
};

// statement is: label, insn (to end of line), or end-of-input
// parser to find point to restart scan after error
auto const resync = *(x3::char_ - stmt_eol) > stmt_eol;

auto const statement_def = 
         (stmt_tuple > stmt_eol)
       | label_tuple
       | x3::eoi > x3::attr(stmt_eoi{})
       ;
       
// insn is statment (after skipping blank or commented lines)
auto const stmt_def  = *stmt_eol > statement;

BOOST_SPIRIT_DEFINE(stmt)
BOOST_SPIRIT_DEFINE(statement)

///////////////////////////////////////////////////////////////////////////
// Annotation and Error handling
///////////////////////////////////////////////////////////////////////////

// annotation is performed `per-parser`. only error-handling is here.

struct resync_base {

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

    //auto message = "\"" + std::string(first_unparsed, first) + "\"";
    //std::cout << "unparsed: " << message << std::endl;

        // return error from parser
        return x3::error_handler_result::fail;
    }

private:
    // parser base error handler
    kas::parser::error_handler_base base;
};

// XXX undef of statement screws up parser 
struct statement : annotate_on_success {};
struct _stmt : resync_base {};
struct _junk : resync_base {};

}
#if 1
namespace kas
{
parser::parser_type stmt()
  //    parser::parser_type const& stmt()
{
    return parser::stmt;
}
}
#endif
#endif
