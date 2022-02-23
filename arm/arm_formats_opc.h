#ifndef KAS_ARM_ARM_FORMATS_OPC_H
#define KAS_ARM_ARM_FORMATS_OPC_H

#include "arm_mcode.h"
#include "target/tgt_format.h"
#include "target/tgt_opc_branch.h"


namespace kas::arm::opc
{
// derive arm version of `tgt` classes
// NB: use `target` default implementations
using fmt_gen    = tgt::opc::tgt_fmt_opc_gen   <arm_mcode_t>;
using fmt_list   = tgt::opc::tgt_fmt_opc_list  <arm_mcode_t>;
using fmt_branch = tgt::opc::tgt_fmt_opc_branch<arm_mcode_t, struct arm_branch>;

// use generic bit inserter/extractor
template <unsigned...Ts>
using fmt32_generic = tgt::opc::tgt_fmt_generic<arm_mcode_t, uint32_t, Ts...>;

template <unsigned...Ts>
using fmt16_generic = tgt::opc::tgt_fmt_generic<arm_mcode_t, uint16_t, Ts...>;

// use generic template to generate argument `mix-in` type
template <unsigned N, typename T>
using fmt_arg = tgt::opc::tgt_fmt_arg<arm_mcode_t, N, T>;

//
// arm specific base opcode formatters
//
static constexpr auto ARM_G0 = kbfd::kbfd_reloc::RFLAGS_ARM_G0;
static constexpr auto ARM_G1 = kbfd::kbfd_reloc::RFLAGS_ARM_G1;
static constexpr auto ARM_G2 = kbfd::kbfd_reloc::RFLAGS_ARM_G2;
static constexpr auto ARM_G3 = kbfd::kbfd_reloc::RFLAGS_ARM_G3;


// special for ARM `BL` instruction
struct arm_branch : tgt::opc::tgt_opc_branch<arm_mcode_t>
{
    void do_calc_size(data_t&                data
                    , mcode_t const&         mcode
                    , mcode_size_t          *code_p
                    , expr_t const&          dest
                    , stmt_info_t const&     info
                    , expression::expr_fits const& fits) const override
    {
        // ARM insns are always one word
        data.size = mcode.code_size();
    }
    
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
        arg.set_mode(*code_p & 0xff);   // retrieve mode (for ARM always DIRECT)
        *code_p &=~ 0xff;               // clear mode

        //arg.set_branch_mode(mcode.calc_branch_mode(data.size()));
        
        // 2. insert `dest` into opcode
        // get mcode validators: displacement always "last" arg
        auto  val_it = mcode.vals().last();
        auto  cnt    = mcode.vals().size();
        auto& fmt    = mcode.fmt();
       
        // insert arg into base insn (via reloc) as required
        if (!fmt.insert(cnt-1, code_p, arg, &*val_it))
            fmt.emit_reloc(cnt-1, base, code_p, arg, &*val_it);
        
        // 3. emit base code
        auto words = mcode.code_size()/sizeof(mcode_size_t);
        for (auto end = code_p + words; code_p < end;)
        {
            // convert mcode_size_t (16-bits) to 32-bits
            uint32_t value = *code_p++ << 16;
            value |= *code_p++;
            base << value;
        }
    }
};

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
            // emit V4BX as `bare reloc` & then regular object data
            static kbfd::kbfd_reloc reloc { kbfd::ARM_REL_V4BX(), 32 };
            base << core::emit_reloc(reloc) << core::emit_reloc::flush();
            base_t::emit(data, base, dot_p);
        }
    };

    // boilerplate to xlate `fmt` to `core::opcode_t`
    arm_mcode_t::opcode_t& get_opc() const override 
    {
        static fmt_opc_bx opc;      // instantiate opcode
        return opc;                 // return reference
    }
};
}

#endif
