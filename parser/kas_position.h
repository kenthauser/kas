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

#include "parser_config.h"
#include <string>

namespace kas::parser
{

struct kas_loc
{
    using index_t   = std::uint32_t;

    constexpr kas_loc(index_t loc = {}) : loc(loc) {}

    // test if `loc` is set
    operator bool() const { return loc; }

    // used in error_handler::where & for ostream
    auto get() const { return loc; }
    std::string where() const;

private:
    index_t loc;
};

template <typename Iter>
struct kas_position_tagged_t 
{
    // calculate & return `kas_loc`
    // NB: implementation at end of `error_reporting.h`
    operator kas_loc&() const;

    // access underlying string.
    auto  begin() const { return first; }
    auto& end()   const { return last;  }
    
    Iter first;
    Iter last;
    error_handler<Iter> const *handler{};
    
private:
    mutable kas_loc loc;
};

}

#endif
