#ifndef KAS_DWARF_DWARF_OPC_H
#define KAS_DWARF_DWARF_OPC_H

#include "kas/kas_string.h"
#include "kas_core/opc_fixed.h"
#include "kas_core/opc_leb.h"
#include "kas_core/core_fixed_inserter.h"
#include "utility/print_type_name.h"

//
// Generate a stream of instructions given the `insn inserter`
//
// The `insn_inserter` requires instances of "core_insn", but impliciate
// conversion from `opcode` is provided via `core_insn` ctor.
//
// /gen_

namespace kas::dwarf
{


// declare argument formats
template <typename NAME, typename OP, int LENGTH = -1>
struct ARG_defn : NAME
{
    using op = OP;
    static constexpr auto size = LENGTH;
};

using namespace kas::core::opc;

using UBYTE = ARG_defn<KAS_STRING("UBYTE"), opc_fixed<uint8_t>,  1>;
using UHALF = ARG_defn<KAS_STRING("UHALF"), opc_fixed<uint16_t>, 2>;
using UWORD = ARG_defn<KAS_STRING("UWORD"), opc_fixed<uint32_t>, 4>;
using ADDR  = ARG_defn<KAS_STRING("ADDR"),  opc_fixed<uint32_t>
                                            , sizeof(DL_STATE::dl_addr_t)>;
using NAME  = ARG_defn<KAS_STRING("NAME"),  opc_string<std::true_type>>;
using TEXT  = NAME;

using ULEB  = ARG_defn<KAS_STRING("ULEB"),  opc_uleb128>;
using SLEB  = ARG_defn<KAS_STRING("SLEB"),  opc_sleb128>;

// create an "core_addr()" entry
// NB: only works if the target fragment is "relaxed"
// XXX see if can refactor to use core_section::for_frags()
// XXX should move to `core_addr`
inline decltype(auto) gen_addr_ref(unsigned section_idx, std::size_t offset
                                  , core::core_fragment const *frag_p = nullptr)
{
    static std::deque<core::addr_offset_t> offsets;

    auto& section = core::core_section::get(section_idx); 
    auto& segment = section[0];
    if (!frag_p)
        frag_p = segment.initial();

    if (!frag_p)
        throw std::runtime_error("gen_addr_ref: empty section");

    // now find fragment for "offset"
    while (frag_p) {
        // is address in this frag?
        // NB: allow at end for zero size or no following frag
        // NB: address is relaxed -- don't let `best` beat `good`
        auto end = frag_p->base_addr()() + frag_p->size()();
        if (offset <= end)
            break;

        // advance to next    
        frag_p  = frag_p->next_p();
    }

    if (!frag_p)
        throw std::runtime_error("gen_addr_ref: offset out of range");

    // save offset in static area
    offset -= frag_p->base_addr()();
    offsets.emplace_back(offset);

    // create address with frag_p/offset_p pair
    return core::core_addr_t::add(frag_p, &offsets.back());
}
}

#endif
