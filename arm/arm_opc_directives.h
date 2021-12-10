#ifndef KAS_ARM_ARM_OPS_H
#define KAS_ARM_ARM_OPS_H

#include "arm_addr_mapping.h"
#include "arm_opc_eabi.h"
#include "target/tgt_directives_impl.h"
#include "utility/ci_string.h"


#include <ostream>

namespace kas::arm::opc
{
using tgt_dir_opcode = parser::tgt_dir_opcode;

struct arm_opc_cpu : tgt_dir_opcode
{
    OPC_INDEX();
    const char *name() const override { return "CPU"; }
    
    void tgt_proc_args(data_t& data, parser::tgt_dir_args&& args) const override
    {
        // create parser to xlate names to eabi defns
        // NB: also instantiates `eabi_tag_t` definitions
        static auto x3 = detail::eabi_tag_parser_t().x3();

        if (auto err = validate_min_max(args, 1, 1))
            return make_error(data, err);
     
        auto p = detail::eabi_tag_t::lookup(5); //"CPU_name");
        arm_opc_eabi::attribute name;
        name.name = args[0].src();
        std::cout << "opc_cpu: name = " << name << ", p = " << p << std::endl;
        print_type_name("opc_cpu::p")(*p);
        print_type_name("opc_cpu::name")(name);
        arm_opc_eabi::get_values().emplace(p, name);
    } 
};
    
struct arm_opc_arch: tgt_dir_opcode
{
    OPC_INDEX();
    const char *name() const override { return "ARCH"; }
};

struct arm_opc_fpu: tgt_dir_opcode
{
    OPC_INDEX();
    const char *name() const override { return "FPU"; }
};
    
//
// keep track of ARM, THUMB, DATA, etc code segments
//
struct arm_opc_arm: tgt_dir_opcode
{
    OPC_INDEX();
    const char *name() const override { return "ARM"; }

    void tgt_proc_args(data_t& data, parser::tgt_dir_args&& args) const override
    {
        if (auto err = validate_min_max(args, 0, 0))
            return make_error(data, err);
       
        arm_seg_mapping()(ARM_SEG_ARM());
    } 
    
};
struct arm_opc_thumb: tgt_dir_opcode
{
    OPC_INDEX();
    const char *name() const override { return "THUMB"; }

    void tgt_proc_args(data_t& data, parser::tgt_dir_args&& args) const override
    {
        if (auto err = validate_min_max(args, 0, 0))
            return make_error(data, err);
       
        arm_seg_mapping()(ARM_SEG_THUMB());
    } 
    
};

struct arm_opc_code: tgt_dir_opcode
{
    OPC_INDEX();
    const char *name() const override { return "CODE"; }

    void tgt_proc_args(data_t& data, parser::tgt_dir_args&& args) const override
    {
        if (auto err = validate_min_max(args, 1, 1))
            return make_error(data, err);
       
        // require numeric argument
        auto p = args.front().get_fixed_p();
        if (p) switch (*p)
        {
            case 16:
                parser::arm_insn_t::set_arch(SZ_ARCH_THB);
                return arm_seg_mapping()(ARM_SEG_THUMB());
            case 32:
                parser::arm_insn_t::set_arch(SZ_ARCH_ARM);
                return arm_seg_mapping()(ARM_SEG_ARM());
        }
        return make_error(data, error_msg::ERR_argument, args[0]);
    } 
    
};
    
struct arm_opc_t_func : tgt_dir_opcode
{
    OPC_INDEX();
    const char *name() const override { return "T_FUNC"; }

    void tgt_proc_args(data_t& data, parser::tgt_dir_args&& args) const override
    {
        if (auto err = validate_min_max(args, 0, 0))
            return make_error(data, err);
    } 
    
};
    
//
// control parser interpretation of address modes, etc
//
struct arm_opc_syntax: tgt_dir_opcode
{
    OPC_INDEX();
    const char *name() const override { return "SYNTAX"; }

    // provide method to interpret args
    void tgt_proc_args(data_t& data, parser::tgt_dir_args&& args) const override
   {
       if (auto err = validate_min_max(args, 1, 1))
           return make_error(data, err);
        auto s = ci_string(args[0].begin(), args[0].end());
        if (s.compare("unified"))
            ; // drive unified
        else if (s.compare("divided"))
            ; // drive divided
        else
            return make_error(data, error_msg::ERR_argument, args[0]);
   }
};
    
}
#endif
