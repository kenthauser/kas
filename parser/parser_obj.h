#ifndef KAS_PARSER_PARSER_OBJ_H
#define KAS_PARSER_PARSER_OBJ_H

#include "parser.h"
#include "error_handler.h"
#include "parser_stmt.h"
#include "parser_variant.h"

#include <boost/filesystem.hpp>

// development path:
// - ctor for parser: container + fs::path
// - ctor creates "error_handler". method to access
// - create "insn_parse_error"
// - create `iter_t` class to perform parser & test eof
namespace kas::parser
{
namespace fs = boost::filesystem;
namespace x3 = boost::spirit::x3;

namespace detail
{
using Iter = kas::parser::iterator_type;


struct parser_src
{
    using value_type = typename Iter::value_type;

private:
    // describe object (eg file) to be parsed
    struct src_obj
    {
        src_obj(Iter const& first, Iter const& last, std::string&& fname)
            : iter(first)
            , last(last)
            , e_handler(error_handler<Iter>(first, last, std::move(fname)))
            {
                if (trace)
                    *trace << "parser_src: add: " << e_handler.fname() << std::endl; 
            }

        auto where(kas_position_tagged loc) const
        {
            return e_handler.where(loc);
        }
        
        //value_type do_parse(kas_parser& parser);
        //void gen_eoi(value_type&) const;
        //bool done() const { return iter == last; }
        
    private:
        Iter iter;
        Iter last;
        error_handler_type e_handler;
    };

    // create a stack of source objects 
    static auto& obstack() {
        // NB: std::deque or std::list
        static auto *_obstack = new std::list<src_obj>;
        return *_obstack;
    }

    // parser public interface via begin/end `iter` pair. Define `iter`
    struct iter_t : std::iterator<std::input_iterator_tag, value_type>
    {
        // NB: iter is `at eof` or not. 
        iter_t(parser_src *src = {}) : src(src) {}

        // money function
        value_type operator*();

        // traditional `nop` functions
        iter_t& operator++() { return *this; }
        void operator++(int) {}

        bool operator!=(iter_t const& it) const
        {
            return src != it.src;
        }

    private:
        parser_src *src;
    };

public:
    // push object into stream (eg: include)
    template <typename...Ts>
    static void add(Ts&&...ts)
    {
        obstack().emplace_back(std::forward<Ts>(ts)...);
    }

    
    // allow iteration over statements
    auto begin()       { return iter_t{this}; }
    auto end() const   { return iter_t{};     }
    auto where(kas_position_tagged loc) const
    {
        return obstack().back().where(loc);
    }

    // trace begin/end of src_obj 
    static void set_trace(std::ostream *out)
    {
        trace = out;
    }

private:
    static inline std::ostream *trace;
};


struct kas_parser 
{
    // ctor
    kas_parser(parser_src src)
        : src(src) {}

private:
    parser_src src;
};

#if 0
// extract statement from current input.
auto inline kas_parser::parser_obj::do_parse(kas_parser& p) -> value_type
{
    value_type ast;

    auto const skipper = as_parser(skipper_t{});
    auto e_handler_ref = std::ref(e_handler);
    auto skipper_ctx   = x3::make_context<x3::skipper_tag>(skipper);
    auto context = x3::make_context<kas::parser::error_handler_tag>(e_handler_ref, skipper_ctx);

    auto before = iter;

    bool success = p.parser.parse(iter, last, context, skipper_t{}, ast);

    if (iter == before) {
        e_handler(p.out, iter, "Error! No input consumed here:");
        iter = last;
        success = false;
    }
    
    if (!success) {
        // create an error insn
        //ast = stmt_error{ parser::kas_diag::last().ref() };
        ast = stmt_error{ parser::kas_diag::last().ref() };
        e_handler.tag(ast, before, iter);
    }
    return ast;
}

void inline kas_parser::parser_obj::gen_eoi(value_type& stmt) const
{
        stmt = stmt_eoi();
        e_handler.tag(stmt, iter, last);
}

// perform parse to extract next statement from current `source`
auto inline kas_parser::iter_t::operator*() -> value_type
{
    auto& s = obstack();

    do {
        // retrieve next instruction from current source.
        auto& back = s.back();
        if (!back.done())
            return back.do_parse(*parser);
           
        // last source ended. resume with previous source.
        // save eoi token, if needed
        // XXX back.gen_eoi(eoi);
        s.pop_back();
    } while (!s.empty());

    // end of input on last source file -- return EOI
    *this = parser->end();      // mark end
    return stmt_eoi();
}
#endif
}

//using detail::parser_obj;
using detail::parser_src;
}

#endif
