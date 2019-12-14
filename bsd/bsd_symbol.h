#ifndef KAS_BSD_BSD_SYMBOLS_H
#define KAS_BSD_BSD_SYMBOLS_H

///////////////////////////////////////////////////////////////////////////
//
//                      B S D _ S Y M B O L S
//
///////////////////////////////////////////////////////////////////////////
//
// The `bsd_ident` is the interface between the parser & `core_symbol` type.
//
// The "symbol table" lookups by name are done in this module. After creation
// `core_symbol` entries are accessed via a `symbol_ref` object.
//
///////////////////////////////////////////////////////////////////////////
//
//
///////////////////////////////////////////////////////////////////////////

#include "kas_core/core_symbol.h"
#include "parser/kas_token.h"

#include <boost/spirit/home/x3.hpp>
#include <map>

namespace kas::bsd
{
namespace x3 = boost::spirit::x3;
using ::kas::parser::kas_token;

//
// "normal" identifers: eg: "[alpha][alphanum*]"
//

struct bsd_ident
{
    // declare symbol & reference types
    using symbol_type = core::core_symbol_t;
    using value_type  = std::add_pointer_t<symbol_type>;

private:
    static auto& sym_table()
    {
        static auto _symtab = new std::map<std::string, value_type>;
        return *_symtab;
    }

public:
    static auto get(kas_token const& token)
    {
        std::string ident = token;
        auto& sym_p       = sym_table()[ident];

        if (!sym_p) {
            // insert took place. create the symbol.
            sym_p = &symbol_type::add(ident, core::STB_TOKEN);
        }

        return sym_p->ref(token);
    }

private:
    static void clear()
    {
        //std::cout << "bsd_ident: clear" << std::endl;
        sym_table().clear();
    }

    static inline core::kas_clear _c{clear};
};

//
// "local" identifers: "[n+]$"
//
// Resets after each "normal" identifier
//

struct bsd_local_ident
{
    using symbol_type = typename bsd_ident::symbol_type;
    using value_type  = typename bsd_ident::value_type;

private:
    static auto& sym_table()
    {
        static auto _symtab = new std::map<int, value_type>;
        return *_symtab;
    }

    static auto& last(bool reset = false)
    {
        static std::string _last = "[initial]";
        if (reset)
            _last = "[initial]";

        return _last;
    }

public:
    // new "normal" ident seen. clear "local" symbol table
    template <typename Context>
    static void set_last(Context const& ctx)
    {
        auto& tok = x3::_attr(ctx);
        last() = tok;
        sym_table().clear();
        x3::_val(ctx) = tok;
    }
    
    static auto get(kas_token const& token, unsigned n)
    {
        auto& sym_p  = sym_table()[n];

        if (!sym_p) {
            // insert took place. create the symbol.
            auto ident = last() + ":" + std::to_string(n);
            sym_p = &symbol_type::add(ident, core::STB_INTERNAL);
        }

        return sym_p->ref(token);
   }

private:
    static void clear()
    {
        //std::cout << "bsd_local_ident: clear" << std::endl;
        last(true);
        sym_table().clear();
    }

    static inline core::kas_clear _c{clear};
};

//
// "numeric" identifiers: "[0-9][bf]"
//
// a set of ten forward and backward identifers to single digit
// numeric label
//

struct bsd_numeric_ident
{
    using symbol_type = typename bsd_ident::symbol_type;
    using value_type  = typename bsd_ident::value_type;

private:
    struct numeric_label
    {
        value_type syms[2];
        unsigned count;     // used to generate symbol name

        void advance() {
            syms[0] = syms[1];
            syms[1] = {};    // create empty symbol
            ++count;
        }
    };

    static inline std::array<numeric_label, 10> labels;

    static auto lookup(unsigned n, bool fwd)
    {
        auto& l = labels[n];
        auto& sym_p = l.syms[fwd];

        if (!sym_p) {
            // first reference:: allocate new symbol
            auto ident = std::to_string(n) + ":" + std::to_string(l.count);
            sym_p = &symbol_type::add(ident, core::STB_INTERNAL);
        }

        return sym_p;
    }

public:

    // two ways to lookup numeric symbol:
    //  - operator() method for label definition
    //  - get() method for token lookup

    template <typename Context>
    void operator()(Context const& ctx) const
    {
        // attribute is "kas_token"
        auto&& token = x3::_attr(ctx);
        
        // token parsed a single digit.
        auto  n = *token.begin() - '0';

        // label definition advances label numbers
        labels[n].advance();

        // lookup & return
        auto sym_p = lookup(n, false); 
        x3::_val(ctx) = sym_p->ref(token);
    }

    static auto get(kas_token const& token, unsigned n, bool fwd)
    {
        auto   sym_p = lookup(n, fwd);
        return sym_p->ref(token);
    }

private:
    static void clear()
    {
        //std::cout << "bsd_numeric_ident: clear" << std::endl;
        for (auto& label : labels)
            label = {};
    }

    static inline core::kas_clear _c{clear};
};

}

#endif
