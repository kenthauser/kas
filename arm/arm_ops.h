#ifndef KAS_ARM_ARM_OPS_H
#define KAS_ARM_ARM_OPS_H

#include "kas_core/opcode.h"


#include <ostream>

namespace kas::arm::parser
{

struct arm_opc_cpu : core::opc::opcode
{
    OPC_INDEX();
    const char *name() const override { return "CPU"; }
    
};
    
struct arm_opc_eabi : core::opc::opcode
{
    OPC_INDEX();
    const char *name() const override { return "EABI"; }
    
};
    
struct arm_opc_arch: core::opc::opcode
{
    OPC_INDEX();
    const char *name() const override { return "ARCH"; }
    
};
    
struct arm_opc_arm: core::opc::opcode
{
    OPC_INDEX();
    const char *name() const override { return "ARM"; }
    
};
    
struct arm_opc_syntax: core::opc::opcode
{
    OPC_INDEX();
    const char *name() const override { return "SYNTAX"; }
    
};
    
struct arm_opc_fpu: core::opc::opcode
{
    OPC_INDEX();
    const char *name() const override { return "FPU"; }
    
};
    
}
#endif
