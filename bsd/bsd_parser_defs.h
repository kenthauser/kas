#ifndef KAS_BSD_BSD_PARSER_DEFS_H
#define KAS_BSD_BSD_PARSER_DEFS_H

// public interface to the bsd parser object
//#include "parser/parser.h"
#include "bsd_insn.h"
#include <boost/spirit/home/x3.hpp>

namespace kas
{
    namespace x3 = boost::spirit::x3;

    namespace bsd::parser
    {
        using namespace kas::parser;
#if 1
        // declare statment parsers
        using pseudo_stmt_x3 = x3::rule<class _pseudo, bsd_stmt_pseudo>;
        BOOST_SPIRIT_DECLARE(pseudo_stmt_x3);
        
        using dwarf_stmt_x3  = x3::rule<class _dwarf,  bsd_stmt_pseudo>;
        BOOST_SPIRIT_DECLARE(dwarf_stmt_x3);
        
        using equ_stmt_x3    = x3::rule<class _equ,    bsd_stmt_equ>;
        BOOST_SPIRIT_DECLARE(equ_stmt_x3);
        
        using org_stmt_x3    = x3::rule<class _org,    bsd_stmt_org>;
        BOOST_SPIRIT_DECLARE(org_stmt_x3);

        using label_stmt_x3  = x3::rule<class _label,  bsd_stmt_label>;
        BOOST_SPIRIT_DECLARE(label_stmt_x3);
    }
#endif
    // parser public interface
    namespace parser::detail {
#if 1
        // declare default BSD comment & separator values
        template<typename = void> struct fmt_separator_str : string<'!'> {};
        template<typename = void> struct fmt_comment_str   : string<';'> {};

        // use BSD comment & separator values as system values
        template<> struct stmt_separator_str<void> : fmt_separator_str<> {};
        template<> struct stmt_comment_str<void>   : fmt_comment_str<>   {};

        // target types for stmt variant
        template <> struct parser_type_l<defn_fmt> : 
            meta::list<
                  bsd::bsd_stmt_pseudo
                , bsd::bsd_stmt_label
                , bsd::bsd_stmt_equ
                , bsd::bsd_stmt_org
                > {};
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
#endif
    }
}


#endif
