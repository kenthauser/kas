#ifndef KAS_Z80_Z80_OPC_DJNZ_H
#define KAS_Z80_Z80_OPC_DJNZ_H

// Z80: djnz dest
//
// if dest offset is within standard offset, emit as <0x10> <disp>
//
// if dest offset is outside standard offset, emit as
//
//  djnz +2; jr +3; jmp dest; ...;

#include "z80_opc_branch.h"

namespace kas::z80::opc
{
struct z80_opc_djnz : tgt::opc::tgt_opc_branch<z80_mcode_t>
{
    // create new opcode: `OPC_INDEX` required!
    OPC_INDEX();
    using NAME = KAS_STRING("Z80_DJNZ");
    const char *name() const override { return NAME::value; }

    void do_initial_size(data_t&            data
                       , mcode_t const&     mcode
                       , expr_t const&      dest
                       , stmt_info_t const& info) const override
    {
        data.size = { 2, 7 };
    }

    void do_calc_size(data_t&                data
                    , mcode_t const&         mcode
                    , mcode_size_t          *code_p
                    , expr_t const&          dest
                    , stmt_info_t const&     info
                    , core::core_fits const& fits) const override
    {
        // check word offset (from end of insn)
        switch(fits.disp_sz(1, dest, 2))
        {
            case expression::NO_FIT:
                data.size.min = data.size.max;
                break;
            case expression::DOES_FIT:
                data.size.max = data.size.min;
                break;
            default:
                break;
        }
    }

    void do_emit     (data_t const&          data
                    , core::emit_base&       base
                    , mcode_t const&         mcode
                    , mcode_size_t          *code_p
                    , expr_t const&          dest
                    , stmt_info_t const&     info) const override
    {
        if (data.size() > 2)
        {
            base << *code_p       << (uint8_t)2;     // djnz +2 (over jr)
            base << (uint8_t)0x18 << (uint8_t)3;     // jr +3   (over jmp)
            base << (uint8_t)0xc3 << core::set_size(2) << dest;  // jmp <dest>
        }

        else
        {
            // emit simple djnz...
            base << *code_p << core::emit_disp(1, -1) << dest;
        }
    }
};

}

#endif

