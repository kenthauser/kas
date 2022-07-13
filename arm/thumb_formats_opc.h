#ifndef KAS_ARM_THUMB_FORMATS_OPC_H
#define KAS_ARM_THUMB_FORMATS_OPC_H



// same namespace as `arm_formats`. get base definitions
#include "arm_formats_opc.h"


namespace kas::arm::opc
{
// derive thumb version of `tgt` classes
// NB: use `target` default implementations
using t16_branch = tgt::opc::tgt_fmt_opc_branch<arm_mcode_t, struct thumb_branch>;


//
// Handle several THUMB branches
//

// special for ARM `BL` instruction
struct thumb_branch : tgt::opc::tgt_opc_branch<arm_mcode_t>
{
#if 0
    void do_calc_size(data_t&                data
                    , mcode_t const&         mcode
                    , mcode_size_t          *code_p
                    , expr_t const&          dest
                    , stmt_info_t const&     info
                    , expression::expr_fits const& fits) const override
    {
        data.size = mcode.code_size();
    }
#endif
#if 0    
    void do_emit     (data_t const&          data
                    , core::core_emit&       base
                    , mcode_t const&         mcode
                    , mcode_size_t          *code_p
                    , expr_t const&          dest
                    , stmt_info_t const&     info) const override
    {
        //std::cout << "arm_branch: do_emit: dest = " << dest << std::endl;
        // 1. create an "arg" from dest expression
        arg_t arg;
        arg.expr = dest;
        arg.set_mode(*code_p & 0xff);   // retrieve mode
        *code_p &=~ 0xff;               // clear mode

        // 2. insert `dest` into opcode
        auto& val   = *mcode.vals().last();
        auto  cnt   =  mcode.vals().size();
        auto& fmt   =  mcode.fmt();
       
        // insert arg into base insn (via reloc) as required
        if (!fmt.insert(cnt-1, code_p, arg, &val))
            fmt.emit_reloc(cnt-1, base, code_p, arg, &val);
        
        // 3. emit base code
        base << *code_p;
        if (data.size() > 2)
            base << *++code_p;
    }
#endif
};




}
#endif

