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

// ARM5: Addressing Mode 2: various forms of indirect in bottom 12 bits + flags
// This formatter requires validator `val_indir`
// NB: `val_indir` converts DIRECT to [A15 *] w/o modifying offset
template <typename RELOC_OP>
struct fmt_reg_indir_t :  arm_mcode_t::fmt_t::fmt_impl
{
    bool insert(mcode_size_t* op, arg_t& arg, val_t const *val_p) const override
    {
        auto value = val_p->get_value(arg);
        //std::cout << "\nfmt_reg_indir::insert: arg = " << arg << ", value = " << std::hex << value << std::endl;
        op[1] |= value;
        op[0] |= value >> 16;
        
        // if non-zero expression, emit reloc
        return arg.expr.empty();
    }
   
    // associated reloc: R_ARM_ABS12
    // use for unresolved immediate offset or conversion of `DIRECT` to PC-REL
    void emit_reloc(core::core_emit& base
                  , mcode_size_t *op
                  , arg_t& arg
                  , val_t const *val_p) const override
    {
        std::cout << "fmt_reg_indir::emit_reloc" << std::endl;
        static const kbfd::kbfd_reloc reloc { RELOC_OP(), 32 };

        auto r = reloc;     // copy base reloc


        switch (arg.mode())
        {
            case arg_t::arg_mode_t::MODE_DIRECT:
                // handle DIRECT xlated to PC-REL
                // NB: PC-rel is from addr + 8
                r.set(r.RFLAGS_PC_REL);
                base << core::emit_reloc(r, {}, -8) << arg.expr;
                break;

            // handle REG_INDIR with unresolved offset
            case arg_t::arg_mode_t::MODE_REG_INDIR:
                std::cout << "fmt_reg_indir::emit_reloc: arg = " << arg << std::endl; 
                std::cout << "fmt_reg_indir::emit_reloc: expr = " << arg.expr << std::endl;
                base << core::emit_reloc(r) << arg.expr;
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

// ARM5: 24-bit branch:  emit reloc and let KBFD sort out overflow
template <unsigned RELOC_FLAG>
struct fmt_branch24 : arm_mcode_t::fmt_t::fmt_impl
{
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
           throw std::logic_error{"invalid fmt_displacement mode"};

        case arg_t::arg_mode_t::MODE_DIRECT:
        {
            //std::cout << "fmt_displacement: MODE_CALL" << std::endl;
            
            // reloc is 24-bits & pc-relative
            // flag `ARM_G1` maps reloc to "R_ARM_CALL24"
            // flag `ARM_G2` maps reloc to "R_ARM_JUMP24"
            static const kbfd::kbfd_reloc r
                    { kbfd::ARM_REL_A32JUMP(), 32, true, RELOC_FLAG }; 
            
            // displacement is from end of insn after insn...
            base << core::emit_reloc(r, {}, -8) << arg.expr;
            break;
        }
#if 0        
        case arg_t::arg_mode_t::MODE_BRANCH:
        {
            // byte displacement stored in LSBs. Use reloc mechanism
            // NB: all arm branches are DIRECT. included for required MODE
            //std::cout << "fmt_displacement: MODE_JUMP" << std::endl;
            
            // reloc is 24-bits & pc-relative
            // flag `ARM_G2` maps reloc to "R_ARM_JUMP24"
            static const kbfd::kbfd_reloc r 
                    { kbfd::ARM_REL_A32JUMP(), 32, true, ARM_G2 }; 
            
            // displacement is from end of insn after insn...
            base << core::emit_reloc(r, {}, -8) << arg.expr;
            break;
        }
#endif
        }
    }
};
// ARM5: Addressing Mode 1: immediate 12-bit value with 8 significant bits
//       and an `even` shift encoded in 4 bits.
// use KBFD to encode
struct fmt_fixed : arm_mcode_t::fmt_t::fmt_impl
{
    // use KBFD to handle special format immediate
    void emit_reloc(core::core_emit& base
                  , mcode_size_t *op
                  , arg_t& arg
                  , val_t const *val_p) const override
    {
        static const kbfd::kbfd_reloc r { kbfd::ARM_REL_IMMED12() };
        
        // let KBFD deal with argument
        base << core::emit_reloc(r) << arg.expr;
    }
};

// ARM5: add/sub: use KBFD to encode argument
// NB: KBFD routine may convert between "add"/"sub" based on args
// derive from `fmt_fixed` to pick up MASKing etc
struct fmt_a32alu : fmt_fixed
{
    // use KBFD to handle special format immediate
    void emit_reloc(core::core_emit& base
                  , mcode_size_t *op
                  , arg_t& arg
                  , val_t const *val_p) const override
    {
        static const kbfd::kbfd_reloc r { kbfd::ARM_REL_A32ALU() };
        
        // let KBFD deal with argument
        base << core::emit_reloc(r) << arg.expr;
    }
};

}
#endif


