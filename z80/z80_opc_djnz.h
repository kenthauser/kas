#ifndef KAS_Z80_Z80_OPC_DJNZ_H
#define KAS_Z80_Z80_OPC_DJNZ_H

// Z80: djnz dest
//
// if dest offset is within byte offset, emit as <0x10> <disp>
//
// if dest offset is outside standard offset, emit as sequence
//
//  djnz +2; jr +3; jmp dest; ...;
// 
// the sequence is 7 bytes

#include "z80_opc_branch.h"

namespace kas::z80::opc
{
struct z80_opc_djnz : tgt::opc::tgt_opc_branch<z80_mcode_t>
{
    // XXX expr_fits should be in global namespace...
    //using expr_fits = typename expression::expr_fits;

    // create new opcode: `OPC_INDEX` required!
    OPC_INDEX();
    using NAME = KAS_STRING("Z80_DJNZ");
    const char *name() const override { return NAME::value; }

    // XXX report sequence size as `max`
    fits_result do_size(mcode_t const& mcode
                      , argv_t& args
                      , decltype(data_t::size)& size
                      , expr_fits const& fits
                      , stmt_info_t info) const override
    {
        return tgt_opc_branch::do_size(mcode, args, size, fits, info);
    }

    void do_emit(core::core_emit& base
                       , mcode_t const& mcode
                       , argv_t& args
                       , stmt_info_t info) const override
    {
        // hand assemble: "djnz +2; jr +3; jmp"
        // NB: DJNZ opcode is first byte
        static constexpr uint8_t long_djnz[] = { 0x10, 0x02, 0x18, 0x03, 0xc3 };
        
        // test "mode" of dest (BYTE or WORD)
        auto branch_mode =  args[0].mode();

        switch (branch_mode)
        {
            case MODE_DIRECT:   // XXX
            case MODE_BRANCH_BYTE:
                // emit djnz & displacement
                base << long_djnz[0] << core::emit_disp(1, {}, -1) << args[0].expr;
                break;
            case MODE_BRANCH_WORD:
                // convert DJNZ to sequence of insns and use JMP to exit
                for (auto c : long_djnz)
                    base << c;
                base << core::set_size(2) << args[0].expr;
                break;
            default:
                throw std::runtime_error{"z80_opc_djnz: invalid mode"};
                break;
        }
    }
};

}

#endif

