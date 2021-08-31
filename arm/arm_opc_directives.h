#ifndef KAS_ARM_ARM_OPC_DIRECTIVES_H
#define KAS_ARM_ARM_OPC_DIRECTIVES_H

namespace kas::arm::opc
{
    

struct arm_opc_cpu   : tgt_dir_opcode {};
struct arm_opc_eabi   : tgt_dir_opcode {};
struct arm_opc_arch   : tgt_dir_opcode {};
struct arm_opc_arm   : tgt_dir_opcode {};
struct arm_opc_syntax   : tgt_dir_opcode {};
struct arm_opc_fpu   : tgt_dir_opcode {};



}


#endif
