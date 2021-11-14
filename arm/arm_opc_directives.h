#ifndef KAS_ARM_ARM_OPS_H
#define KAS_ARM_ARM_OPS_H

#include "target/tgt_directives_impl.h"
#include "arm_addr_mapping.h"
#include "utility/ci_string.h"


#include <ostream>

namespace kas::arm::opc
{
using tgt_dir_opcode = parser::tgt_dir_opcode;

struct arm_opc_cpu : tgt_dir_opcode
{
    OPC_INDEX();
    const char *name() const override { return "CPU"; }
    
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
       
        detail::arm_seg_mapping()(ARM_SEG_ARM());
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
       
        detail::arm_seg_mapping()(ARM_SEG_THUMB());
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
            case 16:    return detail::arm_seg_mapping()(ARM_SEG_THUMB());
            case 32:    return detail::arm_seg_mapping()(ARM_SEG_ARM());
        }
        return make_error(data, error_msg::ERR_argument, args[0]);
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
