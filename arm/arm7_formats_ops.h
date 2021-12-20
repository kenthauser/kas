#ifndef KAS_ARM_ARM7_FORMATS_IMPL_H
#define KAS_ARM_ARM7_FORMATS_IMPL_H


#include "arm_mcode.h"
#include "arm_formats_opc.h"
#include "target/tgt_format.h"

namespace kas::arm::opc
{
// tinker-toy functions to put args into various places...
// always paired: insert & extract

// ARM5: Addressing Mode 1: Immediate Shifts & Register Shifts
// NB: `val_shift` does encoding/decoding
// XXX should be `fmt_abs`
struct fmt_shifter: arm_mcode_t::fmt_t::fmt_impl
{
    bool insert(mcode_size_t* op, arg_t& arg, val_t const *val_p) const override
    {
        auto value = val_p->get_value(arg);
        op[1] |= value;
        op[0] |= value >> 16;
        return true;
    }
    
    void extract(mcode_size_t const* op, arg_t& arg, val_t const *val_p) const override
    {
        auto value = (op[0] << 16) | op[1];
        val_p->set_arg(arg, value);
    }
};


// ARM5: Addressing Mode 1: immediate 12-bit value with 8 significant bits
// relocations: R_ARM_ALU_SB_Gx{_NC}
struct fmt_fixed : arm_mcode_t::fmt_t::fmt_impl
{
    bool insert(mcode_size_t* op, arg_t& arg, val_t const *val_p) const override
    {
        op[1] |= val_p->get_value(arg);     // 12-bits into LSBs
        return true;
    }
    
    void extract(mcode_size_t const* op, arg_t& arg, val_t const *val_p) const override
    {
        val_p->set_arg(arg, op[1]);
    }
#if 0
    void emit(core::core_emit& base, mcode_size_t *op, arg_t& arg, val_t const *val_p) const override
    {
    }
#endif
};

// ARM5: Addressing Mode 2: various forms of indirect in bottom 12 bits + flags
// requires validator `val_indir`
struct fmt_reg_indir :  arm_mcode_t::fmt_t::fmt_impl
{
    bool insert(mcode_size_t* op, arg_t& arg, val_t const *val_p) const override
    {
        auto value = val_p->get_value(arg);
        //std::cout << "\nfmt_reg_indir::insert: arg = " << arg << ", value = " << std::hex << value << std::endl;
        op[1] |= value;
        op[0] |= value >> 16;

        // if non-constant expression, need reloc
        return arg.expr.get_fixed_p();
    }
   
    // associated reloc: R_ARM_ABS12
    // use for unresolved immediate offset or conversion of `DIRECT` to PC-REL
    void emit_reloc(core::core_emit& base
                  , mcode_size_t *op
                  , arg_t& arg
                  , val_t const *val_p) const override
    {
        using ARM_REL_SOFF12 = kbfd::ARM_REL_SOFF12;
        static const kbfd::kbfd_reloc r_abs   { ARM_REL_SOFF12(), 32, false }; 
        static const kbfd::kbfd_reloc r_pcrel { ARM_REL_SOFF12(), 32, true  }; 

        switch (arg.mode())
        {
            // handle DIRECT xlated to PC-REL
            // NB: PC-rel is from addr + 8
            case arg_t::arg_mode_t::MODE_DIRECT:
                op[0] |= 0xf;       // base register is R15
                base << core::emit_reloc(r_pcrel, -8) << arg.expr;
                break;

            // handle REG_INDIR with unresolved offset
            case arg_t::arg_mode_t::MODE_REG_IEXPR:
                base << core::emit_reloc(r_abs) << arg.expr;
                break;
            default:
            
                // internal error
                break;
        }
    }
    
    void extract(mcode_size_t const* op, arg_t& arg, val_t const *val_p) const override
    {
        auto value = (op[0] << 16) | op[1];
        val_p->set_arg(arg, value);
    }
};

struct fmt_movw : arm_mcode_t::fmt_t::fmt_impl
{
    bool insert(mcode_size_t* op, arg_t& arg, val_t const *val_p) const override
    {
        // let reloc code insert value
        return false;
    }
    
    void extract(mcode_size_t const* op, arg_t& arg, val_t const *val_p) const override
    {
        
    }
#if 0
    void emit(core::core_emit& base, mcode_size_t *op, arg_t& arg, val_t const *val_p)
    {
        static const kbfd::kbfd_reloc reloc { kbfd::ARM_REL_MOVW(), 4 };
        base << core::emit_reloc(reloc) << arg.expr;
    }
#endif
};

// ARM5: 24-bit branch
struct fmt_branch24 : arm_mcode_t::fmt_t::fmt_impl
{
    static constexpr auto ARM_G0 = kbfd::kbfd_reloc::RFLAGS_ARM_G0;
    static constexpr auto ARM_G1 = kbfd::kbfd_reloc::RFLAGS_ARM_G1;
    static constexpr auto ARM_G2 = kbfd::kbfd_reloc::RFLAGS_ARM_G2;

    // branch `machine code` insertions handled by `emit_reloc`
    void emit_reloc(core::core_emit& base
                  , mcode_size_t *op
                  , arg_t& arg
                  , val_t const *val_p) const override
    {
        //std::cout << "fmt_branch24: emit: arg = " << arg << std::endl;
        // calculate size here
        switch (arg.mode())
        {
        default:
            //std::cout << "emit_relocation: bad arg: " << arg << ", mode = " << +arg.mode() << std::endl;
           break;
           throw std::logic_error{"invalid fmt_displacement mode"};

        case arg_t::arg_mode_t::MODE_BRANCH:
        {
            // byte displacement stored in LSBs. Use reloc mechanism
            //std::cout << "fmt_displacement: MODE_JUMP" << std::endl;
            
            // reloc is 24-bits & pc-relative
            // flag `ARM_G2` maps reloc to "R_ARM_JUMP24"
            static const kbfd::kbfd_reloc r 
                    { kbfd::ARM_REL_OFF24(), 32, true, ARM_G1 }; 
            
            // displacement is from end of 2-byte machine code
            base << core::emit_reloc(r, -8, 0) << arg.expr;
            break;
        }
        case arg_t::arg_mode_t::MODE_CALL:
            //std::cout << "fmt_displacement: MODE_CALL" << std::endl;
            
            // reloc is 24-bits & pc-relative
            // flag `ARM_G1` maps reloc to "R_ARM_CALL24"
            static const kbfd::kbfd_reloc r
                    { kbfd::ARM_REL_OFF24(), 32, true, ARM_G0 }; 
            
            // displacement is from end of 2-byte machine code
            base << core::emit_reloc(r, -8, 0) << arg.expr;
            break;
        }
    }
};

// ARM5: add/sub
struct fmt_add_sub : arm_mcode_t::fmt_t::fmt_impl
{
    // branch `machine code` insertions handled by `emit_reloc`
    void emit_reloc(core::core_emit& base
                  , mcode_size_t *op
                  , arg_t& arg
                  , val_t const *val_p) const override
    {
        static const kbfd::kbfd_reloc r { kbfd::ARM_REL_ADDSUB(), 32 };
        
        // let KBFD deal with argument
        //std::cout << "fmt_add_sub: emit: arg = " << arg << std::endl;
        base << core::emit_reloc(r) << arg.expr;
    }
};

}
#endif


