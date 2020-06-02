#ifndef KAS_PARSER_PARSER_OBJ_H
#define KAS_PARSER_PARSER_OBJ_H

#include "parser_types.h"
#include "parser_context.h"
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

struct kas_parser 
{
    using value_type = stmt_t;
    
private:
    using parse_fn_t = bool (*)(kas_parser& obj, Iter&, Iter const&);

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
    template <typename PARSER>
    kas_parser(PARSER const&, parser_src& src)
        : src(src)
        , context{kas_context(*this, src.e_handler())}
    {
        parse_fn = [](kas_parser& obj, Iter& iter, Iter const& end)
            {
                return PARSER{}.parse(iter, end, obj.context(), skipper_t{}, obj.value);
            };
    } 

    auto begin()        { return iter_t{this}; }
    auto end()   const  { return iter_t{};     }

    auto parse(Iter& iter, Iter const& end)
    {
        auto& ctx = x3::get<error_diag_tag>(context());
        return parse_fn(*this, iter, end);
    }

    // parser instance variables
    kas_error_t         err_idx {};
    const char *        err_msg {};
    value_type          value   {};

private:
    parse_fn_t          parse_fn;
    parser_src&         src;
    kas_context         context;
};

// extract statement from current input.
auto inline kas_parser::iter_t::operator*() -> value_type
{
    auto& src = obj_p->src;
    
    // save `iter` before parsing to check for error
    auto before = src.iter();
    
    try
    {
        if (obj_p->parse(src.iter(), src.last()))
            return obj_p->value;
    }

    // hook for "reparse". Apparently x3 steps on return value
    // after "reparsing", so come in thru side door.
    catch (Iter& next)
    {
        src.iter() = next;
        return obj_p->value;
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
        std::cout << "kas_parser: nothing parsed" << std::endl;
        src.iter() = src.last();
    }
    else if (!obj_p->err_idx)
    {
        kas_position_tagged loc { before, src.iter(), &src.e_handler() };
        obj_p->err_idx = kas_diag_t::error("Invalid instruction", loc).ref();
    }
        
    stmt_error err(obj_p->err_idx);

    // clear errors before next instruction
    obj_p->err_idx = {};
    obj_p->err_msg = {};

    return err;
}


}
using detail::kas_parser;
}
#endif
