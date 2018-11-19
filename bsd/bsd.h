#ifndef KAS_BSD_BSD_H
#define KAS_BSD_BSD_H

// public interface to the bsd parser object
#include "parser/parser.h"
#include "bsd_options.h"
#include <boost/spirit/home/x3.hpp>

namespace kas
{
    namespace x3 = boost::spirit::x3;

    namespace bsd::parser
    {
        using namespace kas::parser;
        using stmt_t = parser_stmt;

        // declare statment parsers
        using pseudo_stmt_x3 = x3::rule<class _pseudo, stmt_t>;
        BOOST_SPIRIT_DECLARE(pseudo_stmt_x3);
        
        using dwarf_stmt_x3  = x3::rule<class _dwarf,  stmt_t>;
        BOOST_SPIRIT_DECLARE(dwarf_stmt_x3);
        
        using equ_stmt_x3    = x3::rule<class _equ,    stmt_t>;
        BOOST_SPIRIT_DECLARE(equ_stmt_x3);
        
        using org_stmt_x3    = x3::rule<class _org,    stmt_t>;
        BOOST_SPIRIT_DECLARE(org_stmt_x3);

        using label_stmt_x3  = x3::rule<class _label,  stmt_t>;
        BOOST_SPIRIT_DECLARE(label_stmt_x3);
    }

    // parser public interface
    namespace parser::detail {
        // declare default BSD comment & separator values
        template<typename = void> struct fmt_separator_str : string<'!'> {};
        template<typename = void> struct fmt_comment_str   : string<';'> {};

        // use BSD comment & separator values as system values
        template<> struct stmt_separator_str<void> : fmt_separator_str<> {};
        template<> struct stmt_comment_str<void>   : fmt_comment_str<>   {};

        // declare bsd instruction definitions
        template <> struct stmt_ops_l<defn_fmt> : meta::list<
              bsd::parser::pseudo_stmt_x3
            , bsd::parser::dwarf_stmt_x3
            , bsd::parser::equ_stmt_x3
            , bsd::parser::org_stmt_x3
        > {};

        // declare bsd label definitions
        template <> struct label_ops_l<defn_fmt> : meta::list<
            bsd::parser::label_stmt_x3
        > {};
    }
}


#endif
