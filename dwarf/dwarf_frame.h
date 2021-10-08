#ifndef KAS_DWARF_DWARF_FRAME_H
#define KAS_DWARF_DWARF_FRAME_H

#include "kas/kas_string.h"
#include "dwarf_opc.h"
#include "dwarf_frame_data.h"
#include "dwarf_emit.h"
#include <meta/meta.hpp>

namespace kas::dwarf
{
using namespace meta;

template <typename T>
auto& emit_frame_cie(T& emit)
{
    using core::core_symbol_t;
    using core::core_addr_t;
    using core::opc::opc_label;
    using core::opc::opc_align;

    using dl_addr_t = typename DL_STATE::dl_addr_t;

    // NB: need beginning address for FDE.
    auto& bgn_cie  = emit.get_dot();        // get CIE address
    auto& end_cie  = core_addr_t::add();    // allocate unresolved address 
    
    // section length (not including section length field)
    emit(UWORD(), end_cie - bgn_cie - UWORD::size);
    emit(UWORD(), -1);                      // CIE_id (0xffff'ffff -> CIE)
    emit(UBYTE(), 4);                       // CIE version #
    emit(TEXT(), "");                       // (no) augmentation

    emit(UBYTE(), sizeof(dl_addr_t));       // sizeof address
    emit(UBYTE(), 0);                       // segment selector

    //emit(ULEB(), sizeof(dl_addr_t));        // code alignment factor
    emit(ULEB(),  2);                       // XXX ARM
    emit(SLEB(), -4);                       // XXX ARM data alignment factor
    emit(ULEB(), 14);                       // XXX ARM return address column

    //emit(UBYTE(), ...initial insns...);
    // XXX m68k uint32_t cmds[] = {0x0c, 0x0f, 0x04, 0x98, 0x01};
    // XXX ARM
    uint32_t cmds[] = { 0xc, 0xd, 0 };
    for (auto&& cmd : cmds)
        emit(UBYTE(), cmd);
    
    emit(opc_align(), sizeof(dl_addr_t));   // pad to multiple of addr_size
    emit(end_cie);                          // define address `end_cie`
    return bgn_cie;                         // used by `FDE`
}

// implement ADVANCE_LOC
template <typename T>
void advance_loc(T& emit, unsigned& loc, unsigned new_loc)
{
    int32_t delta = new_loc - loc;
    loc = new_loc;          // update address
    delta /= 2;             // XXX code alignment factor


    // generate appropriate insn based on `delta`
    if (delta == 0)
    {
        // suppress insn if zero delta
        return;
    }
    else if (delta < (1 << 6))
    {      
        // small delta stored in insn
        static constexpr auto code = DWARF_CMDS::value[DF_advance_loc].code;
        emit(UBYTE(), code + delta);
    } 
    else if (delta < (1 << 8))
    {
        static constexpr auto code = DWARF_CMDS::value[DF_advance_loc1].code;
        emit(UBYTE(), code);
        emit(UBYTE(), delta);
    } 
    else if (delta < (1 << 16))
    {
        static constexpr auto code = DWARF_CMDS::value[DF_advance_loc2].code;
        emit(UBYTE(), code);
        emit(UHALF(), delta);
    } 
    else
    {
        static constexpr auto code = DWARF_CMDS::value[DF_advance_loc4].code;
        emit(UBYTE(), code);
        emit(UWORD(), delta);
    }
    loc = new_loc;
}

// emit frame
template <typename T, typename FRAME_INFO>
void emit_dwarf_frame(T& emit, core::core_addr_t const& cie, FRAME_INFO const& f)
{
    using core::core_symbol_t;
    using core::core_addr_t;
    using core::opc::opc_label;
    using core::opc::opc_align;

    // emit FDE
    auto& bgn_fde  = emit.get_dot();        // get FDE address
    auto& end_fde  = core_addr_t::add();    // allocate unresolved address

    auto& start_proc = dw_frame_data::get(f.start);
    auto& end_proc   = dw_frame_data::get(f.start + f.delta);

    // section length (not including section length field)
    emit(UWORD(), end_fde - bgn_fde - UWORD::size);
    emit(ADDR(),  cie);
    emit(ADDR(),  f.begin_addr());
    emit(UWORD(), f.end_addr() - f.begin_addr());


    auto fn = [&reader = dw_frame_data::df_reader()]()
    {
        return *reader++;
    };
    unsigned offset = start_proc.offset()();
    auto emit_cfi = [&emit, &offset, &fn](dw_frame_data const& d)
    {
        auto& cmd = DWARF_CMDS::value[d.cmd];
        
        // advance PC as required
        if (offset < d.offset()())
            advance_loc(emit, offset, d.offset()());

        // emit CFI insn
        // if > 0x80, emit with first data value
        auto code = cmd.code;
        if (code < 0x80)
            emit(ULEB(), code);

        // emit args
        auto n = cmd.arg_c;
        for (auto p = cmd.args; n--; ++p)
        {
            // NB: no cmd with "TEXT" arg has code < 0x80
            if (!strcmp(*p, "TEXT"))
            {
                std::string text{};
                while (auto c = fn())
                    text += c;
                emit(TEXT(), text.c_str());
            }
            else if (!strcmp(*p, "BLOCK"))
            {
                // NB: no cmd with "BLOCK" arg has code < 0x80
                // expression??
            }
            else 
            {
                // integral value
                auto v = dw_frame_data::leb128::read(fn);
                if (code >= 0x80)
                {
                    emit(UBYTE(), code + v);
                    code = 0;
                }
                else if (**p == 'F')
                {
                    // scaled & factored
                    emit(SLEB(), -v >> 2);
                }
                else if (**p == 'S')
                    emit(SLEB(), v);
                else
                    emit(ULEB(), v);
            }

        }
    };

    // don't emit `startproc` nor `endproc` cmds
    // NB: `startproc` and `endproc` don't have args
    dw_frame_data::for_each(emit_cfi, f.start + 1, f.delta - 1);

    emit(opc_align(), 4);                     // pad to multiple of addr_size
    emit(end_fde);                            // define address `end_fde`
}


template <typename Inserter>
void dwarf_frame_gen(Inserter&& inserter)
{
    //std::cout << __FUNCTION__ << std::endl;
    //dw_frame_data::dump(std::cout);
    
    // construct emitter
    emit_insn emit(inserter);

    // generate CIE header. Return "CIE" address used by "FDE"
    auto& cie = emit_frame_cie(emit);

    // for each frame, emit FDE header, prologue, and commands
    dw_frame_data::df_reader(true);     // reset reader
    for (auto& frame : dw_frame_data::frames())
    {
        emit_dwarf_frame(emit, cie, frame);    
    }
}
}
#endif

