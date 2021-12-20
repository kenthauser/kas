#ifndef KAS_ARM_THUMB_FORMATS_OPS_H
#define KAS_ARM_THUMB_FORMATS_OPS_H


#include "thumb_formats_opc.h"

namespace kas::arm::opc::thumb
{

// support reg stored as MSB + 3 LSBs
template <unsigned H, unsigned L>
struct fmt_regh : arm_mcode_t::fmt_t::fmt_impl
{
    bool insert(mcode_size_t* op, arg_t& arg, val_t const *val_p) const override
    {
        auto value = val_p->get_value(arg);
        op[0] |=  ((value & 8) << (H - 3)) + ((value & 7) << L);
        return true;
    }
    
    void extract(mcode_size_t const* op, arg_t& arg, val_t const *val_p) const override
    {
        auto value = op[0];
        auto regh = (value >> (H - 3)) & 8;
        auto regl = (value >> L) & 7;
        val_p->set_arg(arg, regh + regl);
    }
};

// support reloc R_ARM_THM_ABS5 (op = ARM_REL_ABS5)
// designed for validator: `val_indir_5`
struct fmt_indir5 : fmt16_generic<3, 8>
{
    using base_t = fmt16_generic<3, 8>;

    bool insert(mcode_size_t* op, arg_t& arg, val_t const *val_p) const override
    {
        base_t::insert(op, arg, val_p);
        return arg.get_fixed_p();       // need reloc if not fixed
    }

    void emit_reloc(core::core_emit& base, mcode_size_t* op, arg_t& arg, val_t const * val_p) const override
    {
        static kbfd::kbfd_reloc reloc { kbfd::ARM_REL_ABS5(), 16 };
        base << core::emit_reloc(reloc) << arg.expr;
    }
};

// support R_ARM_THM_PC8 (op = ARM_REL_PC8)
// designed for validator val_thm_pc8, INSNs: LDR(3), ADD(5)
struct fmt_pc8 : fmt16_generic<0, 8>
{
    using base_t = fmt16_generic<0, 8>;
#if 0
    bool insert(mcode_size_t* op, arg_t& arg, val_t const *val_p) const override
    {
        base_t::insert(op, arg, val_p);
        return arg.get_fixed_p();       // need reloc if not fixed
    }
#endif
    void emit_reloc(core::core_emit& base, mcode_size_t* op, arg_t& arg, val_t const * val_p) const override
    {
        static kbfd::kbfd_reloc reloc { kbfd::ARM_REL_PC8(), 16, true };
        base << core::emit_reloc(reloc) << arg.expr;
    }
};

// XXX support R_ARM_THM_PC8 (op = ARM_REL_PC8)
// XXX designed for validator val_thm_pc8, INSNs: LDR(3), ADD(5)
struct fmt_jump8 : fmt16_generic<0, 8>
{
    using base_t = fmt16_generic<0, 8>;
#if 0
    bool insert(mcode_size_t* op, arg_t& arg, val_t const *val_p) const override
    {
        base_t::insert(op, arg, val_p);
        return arg.get_fixed_p();       // need reloc if not fixed
    }
#endif
    void emit_reloc(core::core_emit& base, mcode_size_t* op, arg_t& arg, val_t const * val_p) const override
    {
        static kbfd::kbfd_reloc reloc { kbfd::ARM_REL_JUMP8(), 16, true };
        base << core::emit_reloc(reloc) << arg.expr;
    }
};

// XXX support R_ARM_THM_PC8 (op = ARM_REL_PC8)
// XXX designed for validator val_thm_pc8, INSNs: LDR(3), ADD(5)
struct fmt_jump11 : fmt16_generic<0, 8>
{
    using base_t = fmt16_generic<0, 8>;
#if 0
    bool insert(mcode_size_t* op, arg_t& arg, val_t const *val_p) const override
    {
        base_t::insert(op, arg, val_p);
        return arg.get_fixed_p();       // need reloc if not fixed
    }
#endif
    void emit_reloc(core::core_emit& base, mcode_size_t* op, arg_t& arg, val_t const * val_p) const override
    {
        static kbfd::kbfd_reloc reloc { kbfd::ARM_REL_JUMP11(), 16, true };
        base << core::emit_reloc(reloc) << arg.expr;
    }
};

struct fmt_call22 : arm_mcode_t::fmt_t::fmt_impl
{
    using base_t = fmt_impl;
    using emit_value_t = typename core::emit_reloc::emit_value_t;
    using THB_CALL = kbfd::ARM_REL_THB_CALL;

    bool insert(mcode_size_t* op, arg_t& arg, val_t const *val_p) const override
    {
        std::cout << "fmt_call22::insert: arg = " << arg << std::endl;

        // use kbfd routine to insert offset value
        static auto& ops = kbfd::kbfd_reloc{THB_CALL()}.get();
        static constexpr auto MASK = (1 << 11) - 1;
    
        // split offset between 11 bit values
        emit_value_t data{};       // get zeros      // XXX need typedef
        auto err = ops.write(data, val_p->get_value(arg));
        
        // XXX throw on error

        // merge reloc result with opcode data 
        op[0] |= (data >> 16) & MASK;
        op[1] |=  data        & MASK;
        return arg.get_fixed_p();       // need reloc if not fixed
    }

    void emit_reloc(core::core_emit& base, mcode_size_t* op, arg_t& arg, val_t const * val_p) const override
    {
        std::cout << "fmt_call22::emit_reloc: arg = " << arg << std::endl;
        static kbfd::kbfd_reloc reloc {THB_CALL(), 32, true};
        base << core::emit_reloc(reloc) << arg.expr;
    }
};

}
#endif


