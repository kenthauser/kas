#ifndef KAS_PARSER_PARSER_OBJ_H
#define KAS_PARSER_PARSER_OBJ_H

#include "parser.h"
#include "error_handler.h"
#include "parser_stmt.h"
#include "parser_variant.h"
#include "parser_src.h"

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


template <typename PARSER>
struct kas_parser 
{
    using value_type = stmt_t;

private:
    // access parser via begin/end `iter` pair.
    struct iter_t : std::iterator<std::input_iterator_tag, value_type>
    {
        // NB: iter is `at eof` or not. 
        iter_t(kas_parser *obj_p = {}) : obj_p(obj_p) {}

        // money function
        value_type operator*();

        // traditional `nop` functions
        iter_t& operator++() { return *this; }
        void operator++(int) {}

        // comparison is really testing for end-of-input
        bool operator!=(iter_t const& other) const
        {
            auto& src = obj_p->src;
            while (src)
            {
                // if current file not at end, more to parse
                if (src.iter() != src.last())
                    return true;

                // pop current file & check again
                src.pop();
            }

            return false;       // end-of-source -> done
        }

    private:
        kas_parser *obj_p;
    };

public:
    // ctor
    kas_parser(PARSER const&, parser_src& src) : src(src) {} 

    auto begin()        { return iter_t{this}; }
    auto end()   const  { return iter_t{};     }

    static auto& skipper_ctx()
    {
        static auto const skipper = as_parser(skipper_t{});
        static auto const ctx = x3::make_context<x3::skipper_tag>(skipper);
        return ctx;
    }

private:
    parser_src&         src;
    error_diag_type     diag;

    // NB: c++ constructs items in order declared;
    // x3::make_context requires l-values;
    std::reference_wrapper<error_handler_type> err_ref  {src.e_handler()};
    std::reference_wrapper<kas_error_t>        diag_ref {diag};
    diag_context_type   diag_ctx = x3::make_context<error_diag_tag>(diag_ref, skipper_ctx());
    error_context_type  context  = x3::make_context<error_handler_tag>(err_ref, diag_ctx); 
};

// extract statement from current input.
template <typename PARSER>
auto inline kas_parser<PARSER>::iter_t::operator*() -> value_type
{
    auto& src = obj_p->src;
    
    // save `iter` before parssing to check for error
    auto before = src.iter();
    
    value_type ast;
    obj_p->diag = {};
    bool success{};
    try
    {
        success = PARSER{}.parse(src.iter(), src.last(), obj_p->context, skipper_t{}, ast);
    }
    catch (std::exception const& e)
    {
        auto exec_name = typeid(e).name();
        std::ostream& diag = std::cout;
        diag << "\nInternal error: " << exec_name << ": " << e.what() << std::endl;
    }

    if (src.iter() == before)
    {
        // need "can't parse anything" diag
        src.iter() = src.last();
        success = false;
    }
#if 0  
    if (!success)
    {
        // create an error insn
        stmt_error err{ parser::kas_diag::last() };
        src.e_handler().tag(err, before, src.iter());
        return err;
    }
#endif

    if (obj_p->diag)
    {
        std::cout << "kas_parser: diag = " << obj_p->diag << std::endl;
        ast = stmt_error(obj_p->diag);
    }

    return ast;
}


}
using detail::kas_parser;

}
#endif
