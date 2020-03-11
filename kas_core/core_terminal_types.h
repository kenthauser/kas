#ifndef KAS_CORE_TERMINAL_TYPES_H
#define KAS_CORE_TERMINAL_TYPES_H

#include "core_size.h"
#include "ref_loc_t.h"

namespace kas::core
{

// forward declare core types
struct core_section;
struct core_expr_dot;


// `symbol_ref` is an opaque type holding a "reference" to
// a `core_symbol` object. Only `core_symbol` can look inside.
//
// `symbol_ref` can be copied and moved freely. In contrast, `core_symbols`
// can't be copied nor moved. It is generally as cheap to construct,
// move, etc, a `symbol_ref` as the `int` it is.
//
// Implementation: `symbol_ref` holds the `core_symbol` index.

template <typename> struct core_symbol;
template <typename> struct core_addr;
template <typename> struct core_expr;

using symbol_ref  = ref_loc_tpl<core_symbol>;
using addr_ref    = ref_loc_tpl<core_addr>;
using expr_ref    = ref_loc_tpl<core_expr>;

using core_symbol_t = typename symbol_ref::object_t;
using core_addr_t   = typename addr_ref::object_t;
using core_expr_t   = typename expr_ref::object_t;

// NB: when mixing offsets, be careful of C++ promotion rules
// c++ promotion rules will convert smaller unsigned to larger signed

// type to hold frag offset & frag size
using frag_offset_t = offset_t<int16_t>;

// type to hold segment offset (host address size)
using addr_offset_t = offset_t<uint16_t>;

// type to hold expression calculations
using expr_offset_t = offset_t<int32_t>;

// these definitions need to be comaptible with opcode_data::fixed_t;
using core_sym_num_t   = std::uint32_t;
struct core_fragment;

template <>
template <typename OS>
void core::addr_ref::print(OS& os) const
{
    //os << "[addr: " << index << " loc: " << loc().get() << "]";
    os << "[addr: " << index << " _loc: " << _loc.get() << "]";
}

template <>
template <typename OS>
void core::symbol_ref::print(OS& os) const
{
    os << "[sym: " << index << " _loc: " << _loc.get() << "]";
}

}
#endif
