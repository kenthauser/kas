#ifndef KAS_PARSER_KAS_POSITION_H
#define KAS_PARSER_KAS_POSITION_H

// `kas_position_tagged_t`
// `kas_loc`
//
// an extension of the `x3::position_tagged` class to solve two problems
//
// - since `kas` handles multiple files (multiple x3::error_handler<> instances)
//   we need a way to identify which instance to look up a `iter_range`.
//   `kas_loc` values form a single sequence and each `x3::error_handler<>`
//   instance is a subrange of values. Thus, the `kas_loc` value can be
//   used as key to get the `error_handler<>` & `position_tagged` instances.
//
// - `x3::position_tagged` values need to outlast parsed values (eg in symbol
//   table for error message). Use internal std::deque() to store
//   original instances.

// `kas_position_tagged` replaces the `x3` class and holds a `kas_loc`.
// allows `annotate_on_success` to properly record location.

//#include "parser_config.h"
#include <string>

#include <iostream>

namespace kas::parser
{

// forward declare error handler
// KAS extension for multiple files
template <typename Iter> struct error_handler;


struct kas_loc
{
    using index_t   = std::uint32_t;

    constexpr kas_loc(index_t loc = {}) : loc(loc) {}

    // test if `loc` is set
    operator bool() const { return loc; }

    // used in error_handler::where & for ostream
    auto get() const { return loc; }    // XXX delete?
    std::string where() const;

private:
    index_t loc;
};

template <typename Iter>
struct kas_position_tagged_t 
{
    using value_type = typename std::iterator_traits<Iter>::value_type;


    // three constructors: default, from iter, from loc
    kas_position_tagged_t() = default;

    kas_position_tagged_t(Iter first, Iter last, error_handler<Iter> const* handler)
        : first(std::move(first)), last(std::move(last)), handler(handler)
        {}

    kas_position_tagged_t(kas_loc const& loc) : loc(loc.get())
        {
            std::cout << "position_tagged: ctor(loc&): loc = " << loc.get();
            std::cout << " handler = " << handler << std::endl;
        }
    
    // calculate & return `kas_loc`
    // NB: implementation at end of `error_reporting.h`
    operator kas_loc&() const;
    void set_loc(kas_loc const& loc)
    {
        this->loc = loc;
    }

    // access underlying character array
    auto  begin() const { return first; }
    auto& end()   const { return last;  }

    // convenience methods
    operator std::basic_string<value_type>() const
    {
        return { first, last };
    }

    std::basic_string<value_type> where() const
    {
        // if inited from `loc` only `loc` is valid. 
        // XXX should init first/last/handler.
        if (loc)
            return loc.where();
            
        // NB: if `first` & `last` not inited, range will be empty string
        //return std::basic_string<value_type>(first, last);
        return *this;
    }

    // XXX XXX XXX
    // first == Iter() crashes parser

    Iter set_first(Iter first)
    {
    #if 0
        // init to empty
        if (first == Iter())
            this->last = first;
    #endif
        this->first = first;
        loc = {};
        return first;
    }

    Iter set_last(Iter last)
    {
        // XXX should this throw?
        if (first != Iter())
        {
            this->last = last;
            loc = {};
        }
        return last;
    }

    void set_handler(error_handler<Iter> const *handler) 
    {
        this->handler = handler;
    }


protected:
    friend struct kas_token;
    Iter first;
    Iter last;
    error_handler<Iter> const *handler{};
    
private:
    mutable kas_loc loc;
};

}

#endif
