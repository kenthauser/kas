#ifndef KAS_PARSER_PARSER_OBJ_H
#define KAS_PARSER_PARSER_OBJ_H

#include "parser.h"
#include "error_handler.h"
#include "parser_stmt.h"

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
    using value_type = stmt_t;
    

    struct kas_parser {
    private:
        // describe object (eg file) to be parsed
        struct parser_obj {
            parser_obj(Iter const&   first
                     , Iter const&   last
                     , std::string&& fname
                     )
                : iter(first)
                , last(last)
                , e_handler(error_handler<Iter>(first, last, std::move(fname)))
                {}

            auto where(kas_position_tagged loc) const
            {
                return e_handler.where(loc);
            }
            
            value_type do_parse(kas_parser& parser);
            void gen_eoi(value_type&) const;
            bool done() const { return iter == last; }
            
        private:
            Iter iter;
            Iter last;
            error_handler_type e_handler;
        };
    
        // create a stack of parsers
        static auto& obstack() {
            // NB: std::deque or std::list
            static auto *_obstack = new std::list<parser_obj>;
            return *_obstack;
        }

    public:
        // ctor
        kas_parser(parser_type const& parser, std::ostream& out)
            : parser(parser), out(out) {}

        // push object into stream (eg: include)
        template <typename...Ts>
        static void add(Ts&&...ts)
        {
            obstack().emplace_back(std::forward<Ts>(ts)...);
        }

#if 0
        // read object with different parser (eg: macro body)
        template <typename...Ts>
        xxx parse_phrase()
        {
            ....  extract macro defns ....
        }
#endif
        // parser public interface via begin/end `iter` pair. Define `iter`
        struct iter_t : std::iterator<std::input_iterator_tag, stmt_t>
        {
            // NB: iter is `at eof` or not. 
            iter_t(kas_parser *parser = {}) : parser(parser) {}

            // money function
            value_type operator*();

            // traditional `nop` functions
            iter_t& operator++() { return *this; }
            void operator++(int) {}

            bool operator!=(iter_t const& it) const
            {
                return parser != it.parser;
            }

        private:
            kas_parser *parser;
        };

        // allow iteration over statements
        auto begin()       { return iter_t{this}; }
        auto end() const   { return iter_t{};     }
        auto where(kas_position_tagged loc) const
        {
            return obstack().back().where(loc);
        }
    private:
        parser_type   parser;
        std::ostream& out;
    };
    
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

        // XXX add `try` logic so users don't need to.
        value_type eoi;

        do {
            // retrieve next instruction from current source.
            auto& back = s.back();
            if (!back.done())
                return back.do_parse(*parser);
               
            // last source ended. resume with previous source.
            // save eoi token, if needed
            back.gen_eoi(eoi);
            s.pop_back();
        } while (!s.empty());

        // end of input on last source file -- return EOI
        *this = parser->end();      // mark end
        return eoi;
    }
}

//using detail::parser_obj;
using detail::kas_parser;
}

#endif
