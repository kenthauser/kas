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

    void do_initial_size     (data_t&                data
                            , mcode_t const&         mcode
                            , mcode_size_t          *code_p
                            , expr_t const&          dest
                            , stmt_info_t const&     info
                            , expression::expr_fits const& fits) const override
    {
        data.size = { 2, 7 };
    }

    void do_emit     (data_t const&          data
                    , core::core_emit&       base
                    , mcode_t const&         mcode
                    , mcode_size_t          *code_p
                    , expr_t const&          dest
                    , arg_mode_t             arg_mode) const override
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
            // XXX second arg should be `kas_loc_p const *` from dest
            base << *code_p << core::emit_disp(1, {}, -1) << dest;
        }
    }
};

}

#endif

