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

    // test ordering
    bool operator< (kas_loc const& o) const { return loc < o.loc; }

    // used in error_handler::where and for ostream
    auto const& get() const { return loc; }
    std::string where() const;

private:
    index_t loc;
};

template <typename Iter>
struct kas_position_tagged_t 
{
    using value_type = typename std::iterator_traits<Iter>::value_type;

    // four constructors: default, from iter, from loc, from pair of instances
    kas_position_tagged_t() = default;

    kas_position_tagged_t(Iter first, Iter last, error_handler<Iter> const* handler)
        : first(std::move(first)), last(std::move(last)), handler(handler) {}

    kas_position_tagged_t(kas_loc const& loc) : loc(loc.get()) {} 

    kas_position_tagged_t(kas_position_tagged_t const& pos_first
                        , kas_position_tagged_t const& pos_last)
                    : kas_position_tagged_t(pos_first)
    {
        // handle `end` if specified
        if (!loc && !handler)
            *this = pos_last;
        else if (pos_last.handler)
            last = pos_last.last;
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
        return where();
    }

    std::basic_string<value_type> where() const
    {
        // if handler not set, use `loc`
        if (!handler)
            loc.where();

        return { first, last };
    }

protected:
    Iter first;
    Iter last;
    error_handler<Iter> const *handler{};
    
private:
    mutable kas_loc loc;
};

}

#endif
