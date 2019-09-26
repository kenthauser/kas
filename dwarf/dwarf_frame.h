#ifndef KAS_DWARF_DWARF_FRAME_H
#define KAS_DWARF_DWARF_FRAME_H

#include "kas/kas_string.h"
#include "dwarf_emit.h"
#include "dwarf_frame_data.h"
#include <meta/meta.hpp>

namespace kas::dwarf
{
using namespace meta;

template <typename T>
auto& gen_cie_32(T& emit)
{
    using core::core_symbol_t;
    using core::core_addr_t;
    using core::opc::opc_label;
    using core::opc::opc_align;

    using dl_addr_t = typename DL_STATE::dl_addr_t;

    auto& bgn_cie  = core_addr_t::get_dot();
    auto& end_cie  = core_symbol_t::add("");//, core::STB_INTERNAL);
    
    // section length (not including section length field)
    emit(UWORD(), end_cie - bgn_cie - UWORD::size);
    emit(UWORD(), -1);                      // CIE_id (0xffff'ffff -> CIE)
    emit(UBYTE(), 4);                       // CIE version #
    emit(TEXT(), "");                       // augmentation

    emit(UBYTE(), sizeof(dl_addr_t));       // sizeof address
    emit(UBYTE(), 0);                       // segment selector

    emit(ULEB(), 2);                        // code alignment factor
    emit(SLEB(), -4);
    emit(ULEB(), 24);                       // return address register

    //emit(UBYTE(), ...initial insns...);
    uint32_t cmds[] = {0x0c, 0x0f, 0x04, 0x98, 0x01};
    for (auto p = std::begin(cmds); p != std::end(cmds); ++p)
        emit(UBYTE(), *p);

    emit(opc_align(), 4);                     // pad to multiple of addr_size
    emit(opc_label(), end_cie.ref());
    return bgn_cie;
}

template <typename T, typename CIE>
void gen_fde_32(T& emit, df_data const& d, CIE& cie)
{
    using core::core_symbol_t;
    using core::core_addr_t;
    using core::opc::opc_label;
    using core::opc::opc_align;

    using dl_addr_t = typename DL_STATE::dl_addr_t;

    // DOT_NEXT is `dot` *after* next instruction emitted
    auto& bgn_fde  = core_addr_t::get_dot(core_addr_t::DOT_NEXT);
    auto& end_fde  = core_symbol_t::add();

    // section length (not including section length field)
    emit(UWORD(), end_fde - bgn_fde);
    emit(ADDR(),  cie);
    emit(ADDR(),  d.begin_addr());
    emit(UWORD(), d.range());

    // emit commands
    auto iter = df_insn_data::get_iter(d.begin_cmd);
    auto n = d.end_cmd - d.begin_cmd;
    uint32_t loc = d.begin_addr().offset()();

    while (n--) {
        auto new_loc = iter->addr.get().offset()();
        advance_loc(emit, loc, new_loc);
        emit_cf_cmd(emit, *iter++);
    }

    emit(opc_align(), 4);                     // pad to multiple of addr_size
    emit(opc_label(), end_fde.ref());
}

template <typename T>
void advance_loc(T& emit, uint32_t& loc, uint32_t new_loc)
{
    int32_t delta = new_loc - loc;
    loc = new_loc;          // update address
    delta /= 2;             // code alignment factor

#if 1
    // suppress insn if zero delta?
    if (delta == 0)
        return;
#endif

    if (delta < (1<< 6)) {      // small delta stored in insn
        emit(UBYTE(), (int)DF_advance_loc + delta);
    } else if (delta < (1 << 8)) {
        emit(UBYTE(), (int)DF_advance_loc1);
        emit(UBYTE(), delta);
    } else if (delta < (1 << 16)) {
        emit(UBYTE(), (int)DF_advance_loc2);
        emit(UHALF(), delta);
    } else {
        emit(UBYTE(), (int)DF_advance_loc4);
        emit(UWORD(), delta);
    }
}

template <typename T>
void emit_cf_cmd(T& emit, df_insn_data const& d)
{
    switch (d.cmd) {
    case DF_offset:
    {
        emit(UBYTE(), d.cmd + d.arg1);
        auto n = -d.arg2;
             n /= 4;
        
        emit(UBYTE(), n);
        break;
    }
    case DF_def_cfa:
        emit(UBYTE(), (int)d.cmd);
        emit(UBYTE(), d.arg1);
        emit(UBYTE(), d.arg2);

        break;
    }
}

template <typename Inserter>
void dwarf_frame_gen(Inserter inserter)
{
    std::cout << __FUNCTION__ << std::endl;
    
    emit_opc<Inserter> emit(inserter);
    DL_STATE state;

    // generate header. Return "end" label to be emitted at end
    auto& cie = gen_cie_32(emit);

    // iterate thru dwarf_line data instructions to generate FSM program
    auto gen_frame = [&emit, &cie](auto& d)
        {
            gen_fde_32(emit, d, cie);
        };
    df_data::for_each(gen_frame);
}
}
#endif

