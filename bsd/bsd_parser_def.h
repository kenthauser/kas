#ifndef KAS_BSD_PARSER_DEF_H
#define KAS_BSD_PARSER_DEF_H

///////////////////////////////////////////////////////////
//
// BSD pseudo-op parser
//
// handle: pseudo-op instructions with comma seperated args (most)
//         pseudo-op instructions with space separated args (mostly dwarf, but others) 
//         label definitions & lookup (three types)
//         `dot` declaration & definition (eg: . = error + 10)
//         EQU definitions
//         macro definitions
//
// Pseudo-ops must be parsed "after" machine insructions
// to prevent macros from intercepting machine instructions
//
// 
///////////////////////////////////////////////////////////
//
// Identifiers
//
// Properly recognizing identifiers is the most difficult function
// of the BSD parser. Consider the following pseudo-op
//
//  .section    .text, "ax", @progbits
//
// the field ".text" is not a "symbol" but looks much like
//
//  .long .private
//
// Where `.private` is a symbol. (Time will tell local or global).
// Assuming the parser allows symbols to begin with `.` (the default),
// it is difficult to distinguish the cases.
// 
// 
// Solution is to "tokenize" identifiers and then process "tokens"
// according to the semantic requirements when evaluating the instruction
//
// These "tokens" are really just bare `kas_position_tagged` subclasses.
// The `kas_position_tagged` type just holds iterators to begin(), end() w/o
// entering in system table. Thus, "token" can be examined & location tagged
// as required.
//
/////////////////////////////////////////////////////////////

// include `parser.h`, not `bsd_parser_types.h`
#include "parser/parser.h"
#include "bsd_elf_defns.h"
#include "bsd_symbol.h"
#include "pseudo_ops_def.h"

#include "parser/token_parser.h"
#include "parser/annotate_on_success.hpp"

#include <boost/spirit/home/x3.hpp>

namespace kas::bsd::parser
{
using namespace boost::spirit::x3;
using kas::parser::token;

// allowed characters in a BSD identifier (NB: can't begin with digit)
const auto bsd_charset = char_("a-zA-Z_.0-9");

//////////////////////////////////////////////////////////////////////////
// BSD Token definitions
//////////////////////////////////////////////////////////////////////////

// BSD has three types of symbols:
// 1. standard ident -- bsd characters: tokenized above
//    labels are LHS, idents are RHS
//    NB: ident's namespace can be "richer" than label's 
auto const ident = token<token_ident>[!digit >> +bsd_charset];
auto const label = token<token_ident>[!digit >> +bsd_charset];

// 2. local labels have format `n$` (eg: `99$`)
//    these ident's scope restarts with each standard label
auto const l_ident = token<token_local_ident>[uint_ >> '$' >> !bsd_charset];

// 3. numeric labels are single digit followed by `b` or `f` (ie back or forward)
//    don't allow c++ binary character to match (eg: 0b'100'1101)
//    omit value, pick up from matched "source"
auto const n_ident = token<token_numeric_ident>
                    [omit[digit >> char_("bBfF") >> !lit('\'') >> !bsd_charset]];

// parse `dot` as a `token`
auto const dot_ident = token<token_dot>['.' >> !bsd_charset];

// parse "nothing"  as "missing" `token`
// NB: also used for pseudo-ops with no args as "dummy" arg
auto const missing = token<token_missing>[eps];

// parser @ "tokens" (used by ELF)
auto const at_token_initial = omit[char_("@%#")];
auto const at_ident = token<token_at_ident>[(at_token_initial >> !digit) > +bsd_charset];
auto const at_num   = token<token_at_num>  [ at_token_initial >   uint_  > !bsd_charset];

// 
// expose dot and idents to `expr` parsers
//

sym_parser_x3 sym_parser {"sym"};
auto const sym_parser_def = ident | l_ident | n_ident;
BOOST_SPIRIT_DEFINE(sym_parser)

dot_parser_x3 dot_parser {"dot"};
auto const dot_parser_def = dot_ident;
BOOST_SPIRIT_DEFINE(dot_parser)


//////////////////////////////////////////////////////////////////////////
// BSD Label Definitions
//
// Handle normal, local, and numeric label definitions.
//
// NB: perform side effects to get BSD label semantics.
//////////////////////////////////////////////////////////////////////////

// local labels restart each time standard label defined.
auto set_last = [](auto& ctx) { bsd_local_ident::set_last(ctx); };

// XXX probably an X3 bug, but fixes eval
auto XXX_eval = [](auto& ctx) { x3::_val(ctx) = x3::_attr(ctx); };

// label format: ident followed by ':'
auto const ident_label   = (label   >> ':')[set_last];
auto const local_label   = (l_ident >> ':')[XXX_eval];

// for numeric: parse digit as token to get location tagging
auto const numeric_label = (token<kas_token>[omit[digit]] >> ':')[bsd_numeric_ident()];

// parse labels as `symbol_ref`
auto const all_labels = rule<class _, core::symbol_ref> {} =
            ident_label | local_label | numeric_label;

// create label instruction (exposed to top parser) 
// NB: location tagged at top level
stmt_label_x3 stmt_label {"bsd_label"};
auto const stmt_label_def = all_labels[bsd_stmt_label()];
BOOST_SPIRIT_DEFINE(stmt_label)


//////////////////////////////////////////////////////////////////////////
// BSD Assembler Instruction Definitions
//////////////////////////////////////////////////////////////////////////

// location tag "expressions". (tokens are already tagged)
struct _tag_expr : kas::parser::annotate_on_success {};
auto const expr_arg   = rule<_tag_expr, bsd_arg>{} = expr();

// space_arg is tagged expr_arg or any *tokens* except missing
// NB: need named rule for expectation error message
auto const space_arg  = rule<class _, bsd_arg>{"space_arg"}
        = expr_arg | dot_ident | at_ident | at_num;

// comma_arg allows missing
auto const comma_arg = rule<class _, bsd_arg>{"comma_arg"}
        = space_arg | missing;

// NB: allow comma separated or space separated (needed for .type)
// NB: no arguments is parsed as [missing]
auto const space_args = rule<class _, bsd::bsd_args> {}
        = space_arg >> (+space_arg | *(',' > space_arg))
        | repeat(1)[missing];

auto const comma_args = rule<class _, bsd::bsd_args> {}
        = comma_arg % ',';

// NB: `comma_ops` have comma separated args. `space_ops` have space separated 
// Parse actual statements
auto const raw_stmt_comma = rule<class _, bsd_stmt_pseudo> {} =
                    (comma_op_x3 > comma_args)[bsd_stmt_pseudo()];
auto const raw_stmt_space = rule<class _, bsd_stmt_pseudo> {} =
                    (space_op_x3 > space_args)[bsd_stmt_pseudo()];
        
// don't allow missing on "=" statements
auto const raw_stmt_org = rule<class _, bsd_stmt_org> {} =
                    ((omit[dot_ident] >> '=') > space_arg)[bsd_stmt_org()];

auto const raw_stmt_equ = rule<class _, bsd_stmt_equ> {} =
                    ((label           >> '=') > space_arg)[bsd_stmt_equ()];

// add extra parser rule to get tagging.
auto const stmt_comma_def = raw_stmt_comma;
auto const stmt_space_def = raw_stmt_space;
auto const stmt_equ_def   = raw_stmt_equ;
auto const stmt_org_def   = raw_stmt_org;

// interface to top-level parser
stmt_comma_x3  stmt_comma   {"bsd_comma"};
stmt_space_x3  stmt_space   {"bsd_space"};
stmt_equ_x3    stmt_equ     {"bsd_equ"  };
stmt_org_x3    stmt_org     {"bsd_org"  };

BOOST_SPIRIT_DEFINE(stmt_comma, stmt_space, stmt_equ, stmt_org)

struct _tag_com : kas::parser::annotate_on_success{};
struct _tag_spc : kas::parser::annotate_on_success{};
struct _tag_equ : kas::parser::annotate_on_success{};
struct _tag_org : kas::parser::annotate_on_success{};
struct _tag_lbl : kas::parser::annotate_on_success{};
}

#endif
