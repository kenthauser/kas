#ifndef KAS_ARM_ARM_FORMATS_OPC_H
#define KAS_ARM_ARM_FORMATS_OPC_H

#include "arm_mcode.h"
#include "target/tgt_format.h"


namespace kas::arm::opc
{
// derive arm version of `tgt` classes
// use generic bit inserter/extractor
template <unsigned...Ts>
using fmt32_generic = tgt::opc::tgt_fmt_generic<arm_mcode_t, uint32_t, Ts...>;

template <unsigned...Ts>
using fmt16_generic = tgt::opc::tgt_fmt_generic<arm_mcode_t, uint16_t, Ts...>;

// use generic template to generate argument `mix-in` type
template <unsigned N, typename T>
using fmt_arg = tgt::opc::tgt_fmt_arg<arm_mcode_t, N, T>;

// NB: use `target` default implementation (fmt_branch not used)
using fmt_gen    = tgt::opc::tgt_fmt_opc_gen   <arm_mcode_t>;
using fmt_list   = tgt::opc::tgt_fmt_opc_list  <arm_mcode_t>;

//
// arm specific base opcode formatters
//

// special for `BX` instruction
struct fmt_bx : virtual arm_mcode_t::fmt_t
{
    // define OPCODE for `bx` format
    struct fmt_opc_bx : tgt::opc::tgt_opc_general<arm_mcode_t> 
    {
        using base_t = tgt_opc_general<arm_mcode_t>;

        OPC_INDEX()
        const char *name() const override { return "ARM_BX"; }

        void emit(data_t const& data
                , core::core_emit& base
                , core::core_expr_dot const *dot_p) const override
        {
            // emit V4BX reloc & then regular object data
            static kbfd::kbfd_reloc reloc { kbfd::ARM_REL_V4BX(), 32 };
            base << core::emit_reloc(reloc) << core::emit_reloc::flush();
            base_t::emit(data, base, dot_p);
        }
    };

    using opcode_t = typename arm_mcode_t::opcode_t;
    opcode_t& get_opc() const override 
    {
        static fmt_opc_bx opc; 
        return opc;
    }
};
}

#endif
