#ifndef KAS_Z80_Z80_OPC_BRANCH_H
#define KAS_Z80_Z80_OPC_BRANCH_H

// Z80_OPC_BRANCH
//
// special opcode to convert relative branch (1 byte offset)
// to long branch (2 byte address) if offset is out of range
//
// Hardcode Z80 Opcodes : 0x18 is JR, 0xc3 is JMP
// Conditional base code: 0x20 is JR_CC, and 0xc3 is JMP_CC
// For conditional instructions, the condition is shifted 3 bits for both


#include "target/tgt_opc_branch.h"

namespace kas::z80::opc
{
struct z80_opc_branch : tgt::opc::tgt_opc_branch<z80_mcode_t>
{
    void do_emit(core::core_emit& base
                       , mcode_t const& mcode
                       , argv_t& args
                       , stmt_info_t info) const override
    {
        // first see if conditional (ie are there two args)
        int  condition_code = -1;   // values are 0-3, or -1

        // if second are present, first is condition-code
        auto branch_mode =  args[1].mode();
        auto dest_p      = &args[1].expr;

        // if second arg is undefined -- uncondition branch to first arg
        if (branch_mode == MODE_NONE)
        {
            // if unconditional: get mode from first (only) arg
            branch_mode =  args[0].mode();   // get branch_mode from first arg
            dest_p      = &args[0].expr;
        }
        else
        {
            condition_code = args[0].reg_p->value(RC_CC);
        }

        // calculate & emit JR or JMP insn
        uint8_t opcode = 0x18;  // default to uncoditional JR
        switch (branch_mode)
        {
            case MODE_BRANCH_BYTE:
                if (condition_code >= 0)
                    opcode = 0x20 + (condition_code << 3);
                base << opcode << core::emit_disp(1, {}, -1) << *dest_p;
                break;
            case MODE_BRANCH_WORD:
                // convert JR -> JMP
                if (condition_code >= 0)
                    opcode = 0xc2 + (condition_code << 3);
                else
                    opcode = 0xc3;  // unconditional jump
                base << opcode << core::set_size(2) << *dest_p;
                break;
            default:
                throw std::runtime_error{"z80_opc_branch: invalid mode"};
                break;
        }
    }
};

}
#endif

