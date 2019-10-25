#ifndef KAS_BSD_BSD_PARSER_TYPES_H
#define KAS_BSD_BSD_PARSER_TYPES_H

// public interface to the bsd parser objects

#include "bsd_stmt.h"
#include <boost/spirit/home/x3.hpp>

namespace kas::bsd::parser
{
    using namespace kas::parser;

    // declare statment parsers
    using stmt_comma_x3 = x3::rule<class _tag_comma, bsd::bsd_stmt_pseudo>;
    BOOST_SPIRIT_DECLARE(stmt_comma_x3)
    
    using stmt_space_x3 = x3::rule<class _tag_space, bsd::bsd_stmt_pseudo>;
    BOOST_SPIRIT_DECLARE(stmt_space_x3)
    
    using stmt_equ_x3   = x3::rule<class _tag_equ, bsd::bsd_stmt_equ>;
    BOOST_SPIRIT_DECLARE(stmt_equ_x3)
    
    using stmt_org_x3   = x3::rule<class _tag_org, bsd::bsd_stmt_org>;
    BOOST_SPIRIT_DECLARE(stmt_org_x3)

    using stmt_label_x3 = x3::rule<class _tag_lbl, bsd::bsd_stmt_label>;
    BOOST_SPIRIT_DECLARE(stmt_label_x3)
}

// parser public interface
namespace kas::parser::detail
{
    // use BSD comment & separator values as system values
    template<> struct stmt_separator_str<void> : fmt_separator_str<> {};
    template<> struct stmt_comment_str  <void> : fmt_comment_str<>   {};

    // types for stmt variant
    template <> struct parser_type_l<defn_fmt> : meta::list<
              bsd::bsd_stmt_pseudo
            , bsd::bsd_stmt_label
            , bsd::bsd_stmt_equ
            , bsd::bsd_stmt_org
            > {};

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
