#ifndef KAS_BSD_BSD_PARSER_TYPES_H
#define KAS_BSD_BSD_PARSER_TYPES_H

// Boilerplate to allow `statement parser` to accept tgt insns
//
// Each "statment" is placed in `stmt_t` type before being evaluated

// get target stmt definitions

#include "bsd_stmt.h"
#include <boost/spirit/home/x3.hpp>

namespace kas::bsd::parser
{
// declare directive name parsers
using comma_op_x3 = x3::rule<class _bsd_comma, detail::pseudo_op_t const *>;
using space_op_x3 = x3::rule<class _bsd_space, detail::pseudo_op_t const *>;
BOOST_SPIRIT_DECLARE(comma_op_x3, space_op_x3)

// declare statment parsers
using stmt_comma_x3 = x3::rule<class _tag_comma , bsd::bsd_stmt_pseudo *>;
using stmt_space_x3 = x3::rule<class _tag_space , bsd::bsd_stmt_pseudo *>;
using stmt_equ_x3   = x3::rule<class _tag_equ   , bsd::bsd_stmt_equ *>;
using stmt_org_x3   = x3::rule<class _tag_org   , bsd::bsd_stmt_org *>;
using stmt_label_x3 = x3::rule<class _tag_lbl   , bsd::bsd_stmt_label *>;

BOOST_SPIRIT_DECLARE(stmt_space_x3, stmt_comma_x3)
BOOST_SPIRIT_DECLARE(stmt_equ_x3, stmt_org_x3, stmt_label_x3)

}

// parser public interface
namespace kas::parser::detail
{

// declare "name" of `defn_fmt` configuration
// NB: must be one of declared names in "parser/paser_types.h"
template <> struct fmt_defn_name<void> : fmt_type_bsd {};

// parsers for label statements 
template <> struct label_ops_l<defn_fmt> : meta::list<
          bsd::parser::stmt_label_x3
        > {};

// parsers for non-label statements
template <> struct stmt_ops_l<defn_fmt> : meta::list<
          bsd::parser::stmt_comma_x3
        , bsd::parser::stmt_space_x3
        , bsd::parser::stmt_equ_x3
        , bsd::parser::stmt_org_x3
        > {};
}


#endif
