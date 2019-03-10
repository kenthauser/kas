#ifndef KAS_PARSER_PARSER_SRC_H
#define KAS_PARSER_PARSER_SRC_H

#include "parser.h"
#include "error_handler.h"

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
    // describe object (eg file) to be parsed
    struct src_obj
    {
        src_obj(src_obj *prev, Iter const& first, Iter const& last, std::string&& fname)
            : prev(prev)
            , iter(first)
            , last(last)
            , e_handler(error_handler<Iter>(first, last, std::move(fname)))
            {
            }

        auto raw_where(kas_position_tagged loc) const
        {
            return e_handler.raw_where(loc);
        }
        
        Iter iter;
        Iter last;
        error_handler_type e_handler;
        src_obj *prev;
    };


public:
    // push object into stream (eg: include)
    template <typename...Ts>
    static void push(Ts&&...ts)
    {
        // allocate src_obj & save as current
        current = new src_obj(current, std::forward<Ts>(ts)...);
        if (trace)
            *trace << "parser_src: push: " << current->e_handler.fname() << std::endl; 
    }

    void pop()
    {
        auto& obj = *current;
        current   = obj.prev;
        if (trace)
        {
            *trace << "parser_src: end: " << obj.e_handler.fname() << std::endl;
            if (current)
                *trace << "parser_src: resuming: " << current->e_handler.fname() << std::endl;
        }
        
        // deallocate completed object
        delete &obj;
    }
    
    // allow iteration over current object
    operator bool() const { return current;       }
    auto& iter()          { return current->iter; }
    auto& last() const    { return current->last; }

    auto& e_handler() const { return current->e_handler; }

    // trace begin/end of src_obj 
    static void set_trace(std::ostream *out)
    {
        trace = out;
    }


// declare print utility
    template <typename T>
    static auto escaped_str(T&& where)
    {
        std::string output;

        for (auto& c : where) {
            switch (c) {
                case '\t':
                    output.append("[\\t]");
                    break;
                case '\n':
                    output.append("[\\n]");
                    break;
                default:
                    output.push_back(c);
                    break;
            }
        }
        return output;
    }

private:
    static inline src_obj *current;
    static inline std::ostream *trace;
};
}
using detail::parser_src;
}

#endif
