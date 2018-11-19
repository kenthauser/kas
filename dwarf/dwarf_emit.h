#ifndef KAS_DWARF_DWARF_EMIT_H
#define KAS_DWARF_DWARF_EMIT_H

#include "kas/kas_string.h"
#include "kas_core/opc_fixed.h"
#include "kas_core/opc_leb.h"
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
    return core::core_addr::add(frag_p, &offsets.back());
}


template <typename INSN_INSERTER>
struct emit_opc
{
    using INSN_T = typename INSN_INSERTER::value_type;
    using core_insn = core::core_insn;
    
    emit_opc(INSN_INSERTER& bi) : bi(bi) {}

    // emit "named" type as actual data
    template <typename DEFN, typename Arg
            , typename OP = typename  DEFN::op>
    void operator()(DEFN, Arg&& arg)
    {
        //std::cout << " " << std::string(DEFN()) << ": arg = " << expr_t(arg) << std::endl;
        do_fixed_emit<OP>(std::forward<Arg>(arg));
    }

    // XXX handle const char * strings.
    void operator()(NAME, const char *name)
    {
        auto str = expression::kas_string::add(name);
        do_fixed_emit<typename NAME::op>(str);
    }

    // emit other ops (eg labels)
    template <typename OP>
    void operator()(OP&& op)
    {
        do_emit(std::forward<OP>(op));
    }

    void operator()(core::symbol_ref const& sym)
    {
        do_emit(core::opc::opc_label{sym});
    }

private:

    template <typename OP, typename Arg>
    void do_fixed_emit(Arg&& arg)
    {
        //print_type_name{"do_fixed_emit"}.name<OP>();
        OP op;

        // XXX this is a mess. Will clean up a lot with insn 
        // refactor to keep loc & first_data out of `insn` proper.
        
        auto initial_size = core::core_insn::data.size();
#if 0
        auto arg_ins = op.template inserter<expr_t>(di);
        *op.size_p = arg_ins(std::move(arg));
#else
        auto proc_fn = op.gen_proc_one(di);
        *op.size_p = proc_fn(std::move(arg));
#endif

        // do INSN stuff
        core_insn insn {op};
        insn.first = initial_size;
        insn.cnt   = core_insn::data.size() - initial_size;

        do_emit(std::move(insn));
    }

    template <typename OP>
    void do_emit(OP&& op)
    {
        *bi++ = std::forward<OP>(op);
    }

    // init object inserter from ctor
    INSN_INSERTER& bi;

    // get data inserter (which is a static object)
    // XXX query insn_inserter to get data_inserter
    static inline auto& di = INSN_T::data_inserter();
};


template <typename INSN_INSERTER>
struct emit_ostream
{
    using INSN_T = typename INSN_INSERTER::value_t;
    emit_ostream(INSN_INSERTER& bi) : bi(bi) {}
   
    // init object inserter from ctor
    INSN_INSERTER& bi;

    // get data inserter (which is a static object)
    // XXX query insn_inserter to get data_inserter
    static auto& di = INSN_T::data_inserter();
};


}

#endif
