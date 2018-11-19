#ifndef KAS_PARSER_SYM_PARSER_H
#define KAS_PARSER_SYM_PARSER_H

#include "sym_parser_detail.h"

namespace kas::parser
{


template <typename T, typename DEFNS, typename ADDER_T = void, typename TYPES = void>
struct sym_parser_t
{
    // get "real" adder
    using ADDER = meta::if_<std::is_void<ADDER_T>
                          , detail::adder<T>
                          , ADDER_T
                          >;

    // declare X3 symbol parser
    using Encoding = boost::spirit::char_encoding::standard;
    using x3_parser_t = x3::symbols_parser<Encoding, typename ADDER::VALUE_T>;
    
    //
    // Perform all "compile-time" calculations
    //

    // use `XLATE_LIST` to extract types used as indexes
    using xlate_list = detail::xlate_list<ADDER>;

    // if type list provided as arg, don't scan defniitions
    using all_types  = meta::if_<std::is_void<TYPES>
                               , detail::get_all_types<xlate_list, DEFNS>
                               , TYPES
                               >;

    // create instances for all xlate types
    // special CTORs for extracted types in listed `XLATE_LIST` types
    using all_types_defns  = detail::gen_all_defns<xlate_list, all_types>;
    
    // create a constexpr array of definitions with optional `CTOR` from adder.
    using ctor   = detail::ctor<ADDER>;
    using defn_t = detail::init_from_list<T, DEFNS, all_types, ctor>;
    
    // expose symbol definitions
    static constexpr auto sym_defns     = defn_t::value;
    static constexpr auto sym_defns_cnt = defn_t::size;

    //
    // Perform all "run-time" operations
    //

    // allocate parser at runtime
    static inline x3_parser_t *parser;

    // use `ADDER` to initialize parser from `constexpr insns`
    void add() const
    {
        if (!parser)
            parser = new x3_parser_t;
        ADDER{*this}(*parser, sym_defns_cnt);
    }

    // x3::symbols_parser is a "prefix" parser
    // most parsers need to be wrapped in lexeme[x3 >> !alnum].
    // operators are the exception. do at instantiation
    auto& x3_raw() const
    {
        if (!parser)
            add();
        return *parser;
    }

    auto x3() const
    {
        return x3::lexeme[x3_raw() >> !x3::alnum];
    }

    struct deref
    {
        template <typename Context>
        void operator()(Context& ctx)
        {
            x3::_val(ctx) = *x3::_attr(ctx);
        }
    };
    
    auto x3_deref() const
    {
        return x3()[deref()];
    }
};

}

#endif

