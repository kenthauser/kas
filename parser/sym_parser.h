#ifndef KAS_PARSER_SYM_PARSER_H
#define KAS_PARSER_SYM_PARSER_H

// `sym_parser_t` : lookup ASCII string to get value
//
// The `sym_parser_t` is the KAS interface to `x3::symbols_parser` for fixed
// value tables. It has wide use inside `kas` including the following:
//
// lookup of insn names
// lookup of register names
// lookup of expression operators (including aliases such as .or.)
// lookup of pseudo-op names
//
// several of the above involve several instances in `sym_parser_t`.
//
// The basic use is: sym_parser_t<T, DEFNS>
//   where`DEFNS` is a `meta::list` of `N` types which can be instantiated into
//   a constexpr std::array<T, N>. Lookup of `T.name` via the `sym_parser_t` instance
//   yields `const T*`. The constexpr array is available at `sym_parser::sym_defns`.
//   This is what happens with the `default` adder
//
// The principle customization of `sym_parser_t` is though the `ADDER`. This type
// can be passed as a member type of `T` or as a template arg. In addition to 
// `ADDER` performing functions such as adding aliases, it can also specify a `NAME_LIST`
// or an `XLATE_LIST` which specify metafunctions performed on the `DEFNS` before
// definition construction. More info on `XLATE_LIST` is in "sym_parser_xlate.h".
//
// The `x3::symbols_parser` is a "prefix" parser. In other words, it's not a token
// parser as might be expected. Thus, there are three `sym_parser_t` parsers defined:
//
//  x3_raw()    // raw prefix parser: used by expression
//  x3()        // normal token parser: lexeme followed by !x3:alnum
//  deref()     // parse as `x3`. return `const T` instead of `const T*`

#include "init_from_list.h"
#include "sym_parser_detail.h"
#include "sym_parser_xlate.h"

namespace kas::parser
{


template <typename T, typename DEFNS, typename ADDER_TPL = void, typename TYPES = void>
struct sym_parser_t
{
    // get "real" ADDER (from template arg, from `T` member type, or default-generated)
    using ADDER = meta::if_<std::is_void<ADDER_TPL>
                          , detail::adder<T>
                          , ADDER_TPL
                          >;

    // get (or generate) `XLATE_LIST` for types to be instantiated
    // and/or used as indexes
    using xlate_list = detail::xlate_list<ADDER>;

    // declare X3 symbol parser
    using Encoding = boost::spirit::char_encoding::standard;
    using x3_parser_t = x3::symbols_parser<Encoding, typename ADDER::VALUE_T>;
    
    //
    // Perform all "compile-time" calculations
    //

    // if type list provided as arg, don't scan defniitions
    // NB: `all_types` also empty if `xlate_list` default generated
    using all_types  = meta::if_<std::is_void<TYPES>
                               , detail::get_all_types<xlate_list, DEFNS>
                               , TYPES
                               >;

    // create instances for all `xlate_list` types
    // `xlate_list` may have provided special CTORs
    using all_types_defns  = detail::gen_all_defns<xlate_list, all_types>;
    
    // create a constexpr array of definitions with optional `CTOR` from adder.
    using ctor   = detail::ctor<ADDER>;
    using defn_t = init_from_list<T, DEFNS, ctor, all_types>;

    // expose symbol definitions
    static constexpr auto sym_defns     = defn_t::value;
    static constexpr auto sym_defns_cnt = defn_t::size;

    // constructor: add instances to parser
    template <typename...Ts>
    sym_parser_t(Ts&&...ts)
    {

    }

    //
    // Perform all "run-time" operations
    //
#if 0
    // allocate parser at runtime
    static inline x3_parser_t *parser;
#endif
    // use `ADDER` to initialize parser from `constexpr defns`
    void do_add() const
    {
        // only do once
        if (!parser)
        {
            parser = new x3_parser_t;
            ADDER{*this}(*parser, sym_defns_cnt);
        }
    }

    // x3::symbols_parser is a "prefix" parser
    // most parsers need to be wrapped in lexeme[x3 >> !endsym].
    // operators are the exception. do at instantiation
    auto& x3_raw() const
    {
        if (!parser)
            do_add();
        return *parser;
    }

    auto x3() const
    {
        // XXX not right: need to disallow pfx
        //if (!pfx)
            return x3::lexeme[x3_raw() >> !(x3::alnum | '_')];
        //else
            //return x3::lexeme[x3::lit(pfx) >> x3_raw() >> !(x3::alnum | '_')];
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

    // allocate parser at runtime (after all definitions completed
    mutable x3_parser_t *parser {};

};

}

#endif

