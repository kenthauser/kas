#ifndef KAS_CORE_TERMINAL_TYPES_H
#define KAS_CORE_TERMINAL_TYPES_H

#include "core_size.h"
#include "ref_loc_t.h"

namespace kas::core
{

// forward declare core types
struct core_section;
struct core_expr_dot;
struct arg_missing;

#if 0
namespace opc
{
    template <typename T         = kas::expression::ast::expr_t
            , typename Container = std::deque<T>
            >
    struct insn_data;

    template <typename DATA = insn_data<>>
    struct insn_opcode;
    
    // create alias using default types
    using opcode = insn_opcode<>;
}
#endif

// `symbol_ref` is an opaque type holding a "reference" to
// a `core_symbol` object. Only `core_symbol` can look inside.
//
// `symbol_ref` can be copied and moved freely. In contrast, `core_symbols`
// can't be copied nor moved. It is generally as cheap to construct,
// move, etc, a `symbol_ref` as the `int` it is.
//
// Implementation: `symbol_ref` holds the `core_symbol` index.

using symbol_ref  = ref_loc_t<struct core_symbol>;
using addr_ref    = ref_loc_t<struct core_addr>;
using expr_ref    = ref_loc_t<struct core_expr>;
using missing_ref = ref_loc_t<struct arg_missing>;

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

struct arg_missing
{
    missing_ref ref(parser::kas_loc = {}) const { return {}; }

    template <typename...Ts>
    static auto& get(Ts&&...)
    {
        static arg_missing missing;
        return missing;
    }
    
    parser::kas_loc loc;
};

template <>
template <typename OS>
void core::addr_ref::print(OS& os) const
{
    os << "[addr: " << index << " loc: " << loc.get() << "]";
}

template <>
template <typename OS>
void core::symbol_ref::print(OS& os) const
{
    os << "[sym: " << index << " loc: " << loc.get() << "]";
}

template <>
template <typename OS>
void core::missing_ref::print(OS& os) const
{
    os << "[missing loc: " << loc.get() << "]";
}
}
#endif
