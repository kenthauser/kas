#ifndef KAS_Z80_Z80_OPC_BRANCH_H
#define KAS_Z80_Z80_OPC_BRANCH_H


#include "target/tgt_opc_branch.h"

namespace kas::z80::opc
{
struct z80_opc_branch : tgt::opc::tgt_opc_branch<z80_mcode_t>
{
    void do_calc_size(data_t&                data
                    , mcode_t const&         mcode
                    , mcode_size_t          *code_p
                    , expr_t const&          dest
                    , stmt_info_t const&     info
                    , core::core_fits const& fits) const override
    {
        using disp_t = expression::e_data_t;    // displacement type

        // validate opcode elegible for deletion
        while(data.size.min < data.size.max)
        {
            expression::fits_result r;
            uint8_t no_fits_min = data.size.min + 1;
            
            // 
            switch (data.size.min)
            {
                case 0:
                    // test for deletion
                    // see if `offset` is just this insn
                    r = fits.disp(dest, 0, data.size.max, data.size.max);
                    no_fits_min = 2;        // minimum of two bytes if not deleted
                    break;
                default:
                    // first word is INSN, rest is offset
                    // offset from end of INSN
                    // disp_sz calculated to allow 1 byte opcode, rest disp
                    r = fits.disp_sz(data.size.min-1, dest, data.size.min);
            }

            // now test
            std::cout << " result = " << +r << std::endl;
            switch (r)
            {
                case expression::NO_FIT:
                    data.size.min = no_fits_min;
                    continue;
                case expression::DOES_FIT:
                    data.size.max = data.size.min;
                    return;         // done
                default:
                    return;
            }

        }
    }

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
            static constexpr core::core_reloc reloc { core::K_REL_ADD, 0, true };
            base << *code_p;
            base << core::set_size(1) << core::emit_reloc(reloc, -1) << dest << 0;
        }
    }

};


}

#endif

