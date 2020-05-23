#ifndef KAS_Z80_Z80_OPC_BRANCH_H
#define KAS_Z80_Z80_OPC_BRANCH_H


#include "target/tgt_opc_branch.h"

namespace kas::z80::opc
{
struct z80_opc_branch : tgt::opc::tgt_opc_branch<z80_mcode_t>
{
    void do_emit     (data_t const&          data
                    , core::emit_base&       base
                    , mcode_t const&         mcode
                    , mcode_size_t          *code_p
                    , expr_t const&          dest
                    , stmt_info_t const&     info) const override
    {
        if (data.size() == 0)
            return;         // deleted

        // see if need to convert `jr` to `jmp`
        if (data.size() > 2)
        {
            // covert to `jmp`
            auto code = *code_p;
            switch (code)
            {
                // unconditional jr
                case 0x18:      // jr -> jmp
                    code = 0xc3;
                    break;
                // assume jr_cc
                default:
                    // map jr_cc -> jmp cc
                    code = (code & 0x18) | 0xc2;
                    break;
            }
            base << code << core::set_size(2) << dest;
        }

        else
        {
            // emit a branch
            // single word opcode, single word displacement from end of insn
            base << *code_p << core::emit_disp(1, -1) << dest;
        }
    }
};


}

#endif

