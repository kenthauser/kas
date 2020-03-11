#ifndef KAS_PARSER_ERROR_HANDLER_H
#define KAS_PARSER_ERROR_HANDLER_H

// X3 error handler to handle multiple source files.
//
// The X3 parser tags locations with a "begin/end" pair.  This
// is fine for a single file, but assembler inputs typically derive from
// many source files (either from assembler `includes` or more typically from
// `.dwarf file` insns linking to c-source files). To make this work, this module
// creates a 'stack' of native X3-style error handlers, 'pushing' onto the stack when a new
// source file is entered & 'popping' at the end of the source.
//
// Except, the native X3 handlers aren't really "popped", but rather abandoned in
// place, because position tags are still needed for error messages during
// object code assembly.
//
// The native X3 handlers save the two iterators which make the `begin/end` pair
// in an `x3::position_tagged` structure -- which contains the offsets into a std
// container of the actual iterators. The offsets (with member names `id_first` & `id_last`)
// begin with values { 0, 1 } et. al. The `id_*` values of -1 are used to indicate uninitialized.
//
// The combined error handler takes advantage that offset pairs are consecutive in each native
// handler, until beginning a run in the next "pushed" handler. Thus, a single integer can be
// created to identify an entry across multiple files. This integer, the `kas_loc`, is logically 
// incremented each time a "native" x3 position is tagged. It is implemented by storing the current
// `kas_loc` and the current "native" x3 position each time handler is "pushed".
//
// A `std::map` is used to store the "pushed" locations. The "lower_bound" method allows for
// simple translation from a `kas_loc` value to a "native" offset.

#include "kas_position.h"
#include "error_reporting.h"        // x3-style single file handler

#include <map>
#include <deque>
#include <iterator>

namespace kas::parser
{

template <typename Iter>
struct error_handler
{

    using x3_handler = x3_error_handler<Iter>;
    
    // declare index types for "x3_handler" & "kas_loc"
    using hdl_index_t = uint32_t;
    using loc_index_t = typename kas_loc::index_t;

private:
    // build persistent x3 error handlers in container
    static auto& handlers()
    {
        static auto _handlers = new std::deque<x3_handler>;
        return *_handlers;
    }
    

    // map kas_loc to error_handler index
    // store "index" & "offset" for each handler
    struct kas_loc_segment_info
    {
        hdl_index_t idx;       // x3_handler index (+1)
        loc_index_t offset;    // add to x3_kloc for this segment
    };

    static auto& map()
    {
        static auto _map = new std::map<loc_index_t, kas_loc_segment_info>;
        return *_map;
    }

public:
    static auto &get_handler(hdl_index_t idx)
    {
        return handlers()[idx-1];
    }

private:
    // retrieve current error handler
    // NB: not valid if empty!
    auto& top() const
    {
        auto& m = map();
        return std::prev(m.end())->second;
    }

    // add new x3::error_handler index to "stack"
    void push(hdl_index_t idx)
    {
        auto& m = map();

        // declare & set default for initial handler
        loc_index_t last_loc = 0;      // last assigned

        if (!m.empty()) {
            // retrieve last LOC from previous handler
            auto& info = top();
            auto& prev = get_handler(info.idx);
            last_loc   = prev.get_last_loc();
        }
        
        // skip 1 loc for each change in error handler
        // a Zero length segment is still unique
        // NB: also allows late tagging of source line for listing

        ++last_loc;
        
        // set new offset
        auto offset = get_handler(idx).set_next_loc(last_loc);

        // create new segment map entry
        m.emplace_hint(m.end(), last_loc, kas_loc_segment_info{idx, offset});

#undef TRACE_ERROR_HANDLER
#ifdef TRACE_ERROR_HANDLER
        std::cout << "error_handler: entered segment " << m.size();
        std::cout << ", locs from = " << last_loc;
        std::cout << ", handler = " << idx;
        std::cout << ", offset = " << offset << std::endl;
#endif
    }

public:
    // ctor arguments are passed directly to x3_handler
    template <typename...Ts>
    error_handler(Ts&&...args)
    {
        // new include file -- at completion, go back to previous current
        // initial file flagged with previous == "zero"
        prev = map().empty() ? 0 : top().idx;

        // create new x3_error_handler & make active
        auto& vec = handlers();
        handler   = &vec.emplace_back(std::forward<Ts>(args)...);
        push(vec.size());       // make this handler active
    }

    ~error_handler()
    {
        // at end of source file
        // logically pop current handler off the stack...
        // ... by pushing prev as new current
        if (prev)
            push(prev);
    }

    // record iterator locations to allow later retrieval
    template <typename AST>
    void tag(AST& ast, Iter const& first, Iter const& last) const
    {
#ifdef TRACE_ERROR_HANDLER
        std::cout << "error_handler::tag:";
        std::cout << " src = " << escaped_str(std::string(first, last));
        std::cout << std::endl;
        std::cout << "error_handler::tag: AST = ";
        std::cout << boost::typeindex::type_id_with_cvr<AST>().pretty_name();
        std::cout << std::endl;
#endif
        annotate(ast, first, last, std::is_base_of<kas_position_tagged, AST>());
    }

    // This will catch all nodes except those inheriting from `kas_position_tagged`
    template <typename AST>
    void annotate(AST& ast, Iter const& first, Iter const& last, std::false_type) const
    {
#ifdef TRACE_ERROR_HANDLER
        std::cout << "error_handler::annotate: false_type" << std::endl;
#endif
        // XXX add support for `set_loc()` types (eg: ref_loc)
        if constexpr (std::is_base_of<core::ref_loc_tag, AST>())
        {
            print_type_name("annotate::set_loc").name<AST>(std::cout);

            // set loc if unset
            if (!ast.template get_p<parser::kas_loc>()) {
                kas_position_tagged pos(first, last, this);
                get_loc(pos);
            }
        }
        // (no-op) no need for tags
    }

    // This will catch all nodes inheriting from `kas_position_tagged`
    void annotate(kas_position_tagged& ast, Iter const& first, Iter const& last, std::true_type) const 
    {
#ifdef TRACE_ERROR_HANDLER
        std::cout << "error_handler::annotate: true_type" << std::endl;
#endif
    #ifndef XXX
        // splice ast
        ast = { first, last, this };
    #else
        ast.set_first  (first);
        ast.set_last   (last);
        ast.set_handler(this);
    #endif
    }

    // trampoline `kas_position_tagged::get_loc`. Include file issues
    auto get_loc(kas_position_tagged const& ast) const
    {
        return handler->get_loc(ast);
    }

    // get beginning of file for listing
    static auto initial(hdl_index_t idx)
    {
        return get_handler(idx).first();
    }

    // retrieve boost::iter_range from appropriate error handler
    // return std::pair<file-index, boost::iter_range>
    static auto raw_where(kas_loc const& loc)
    {
        auto loc_idx = loc.get();
        if (loc_idx == 0)
            throw std::logic_error("error_handler::where: zero loc");
#ifdef TRACE_ERROR_HANDLER        
        std::cout << "error_handler::where: loc = " << loc_idx << std::endl;
#endif
        auto& info = std::prev(map().upper_bound(loc_idx))->second;
#ifdef TRACE_ERROR_HANDLER
        std::cout << "error_handler::info: idx = " << info.idx;
        std::cout << " offset = " << info.offset << std::endl;
#endif
        auto& handler = get_handler(info.idx);
        return std::make_pair(info.idx, handler.get_src(loc_idx - info.offset));
    }

    // emit error message (with or without source)
    static auto err_message(std::ostream& err_out, kas_loc const& loc
                          , std::string const& message, bool print_line = false)
    {
        auto w = raw_where(loc);
        auto handler = get_handler(w.first);
        return handler(err_out, w.second.begin(), w.second.end(), message, print_line);
    }

    auto fname() const
    {
        return handler->get_file();
    }

    // stream error message (see error_reporting.h for formats)
    // XXX these messages need to be routed thru `kas_diag`
    template <typename...Ts>
    decltype(auto) operator()(Ts&&...args) const
    {
        return (*handler)(std::forward<Ts>(args)...);
    }

private:
    x3_handler  *handler;    // current x3 handler
    hdl_index_t  prev;       // previous handler index
    loc_index_t  offset;     // current offset
};

}

#endif
