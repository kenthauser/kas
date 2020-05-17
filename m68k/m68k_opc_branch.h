#ifndef KAS_M68K_M68K_OPC_BRANCH_H
#define KAS_M68K_M68K_OPC_BRANCH_H


#include "target/tgt_opc_branch.h"

namespace kas::m68k::opc
{
struct val_branch;

struct m68k_opc_branch : tgt::opc::tgt_opc_branch<m68k_mcode_t>
{
};

}

#endif

