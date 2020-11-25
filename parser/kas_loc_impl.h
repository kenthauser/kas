#ifndef KAS_PARSER_KAS_LOC_IMPL_H
#define KAS_PARSER_KAS_LOC_IMPL_H

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
//
// `kas_position_tagged` replaces the `x3` class and holds a `kas_loc`.
// allows `annotate_on_success` to properly record location.

#include "kas_loc.h"
#include "error_handler.h"

namespace kas::parser
{
// XXX std::string is wrong return type
std::string kas_loc::where() const
{
    return escaped_str<char>(src());
}

std::string kas_loc::src() const
{
    if (*this)
    {
        auto w = error_handler_type::raw_where(loc);
        return { w.second.begin(), w.second.end() };
    }
    return {};
}

bool kas_loc::operator < (kas_loc const& o) const
{
    //std::cout << "kas_loc::operator<: " << loc << " (" << where() << ") < ";
    //std::cout << o.loc << " (" << o.where() << ")" << std::endl;

    // NB: this test is designed to test if 
    // 1) source lines are ordered,
    // 2) if "diagnostic" is located in a source line.
    //
    // Thus, test the `end` first & use as `!(diag < line)` to test (line <= diag)

    auto w   = error_handler_type::raw_where(loc);
    auto o_w = error_handler_type::raw_where(o.loc);

    if (w.second.begin()  < o_w.second.end())
        return true;
    return w.second.end() < o_w.second.end();
}
}

// declare `kas_error_t` diagnostic printer
namespace kas::core
{
template <>
template <typename OS>
void parser::kas_error_t::print(OS& os) const
{
    auto& obj = get();
    os << "["  << "kas_error_t: "   << std::dec << index;
    os << ": " << ": " << obj.level_msg() << obj.message;
    os << ": " << " loc: " << _loc.get();
    os << ": " << _loc.where();
    os << "]"  << std::flush;
}

}

#endif
