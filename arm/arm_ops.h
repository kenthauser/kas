#ifndef KAS_ARM_ARM_OPS_H
#define KAS_ARM_ARM_OPS_H

#include "target/tgt_directives_impl.h"


#include <ostream>

namespace kas::arm::opc
{
using tgt_dir_opcode = parser::tgt_dir_opcode;

struct arm_opc_cpu : tgt_dir_opcode
{
    OPC_INDEX();
    const char *name() const override { return "CPU"; }
    
};
    
struct arm_opc_eabi : tgt_dir_opcode
{
    OPC_INDEX();
    const char *name() const override { return "EABI"; }
    
};
    
struct arm_opc_arch: tgt_dir_opcode
{
    OPC_INDEX();
    const char *name() const override { return "ARCH"; }
    
};
    
struct arm_opc_arm: tgt_dir_opcode
{
    OPC_INDEX();
    const char *name() const override { return "ARM"; }
    
};
    
struct arm_opc_syntax: tgt_dir_opcode
{
    OPC_INDEX();
    const char *name() const override { return "SYNTAX"; }
    
};
    
struct arm_opc_fpu: tgt_dir_opcode
{
    OPC_INDEX();
    const char *name() const override { return "FPU"; }
    
};
    
}
#endif
