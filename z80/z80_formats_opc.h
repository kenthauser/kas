#ifndef KAS_Z80_Z80_FORMATS_OPC_H
#define KAS_Z80_Z80_FORMATS_OPC_H

#include "z80_formats_type.h"

#include "opc_general.h"
#include "opc_list.h"
#include "opc_branch.h"
#include "opc_dbcc.h"

namespace kas::z80::opc
{

// general opcode format
struct fmt_gen : virtual z80_opcode_fmt
{
    static inline z80_opc_general opc; 
    virtual z80_stmt_opcode& get_opc() const override 
    {
        return opc;
    }
};

// list opcode format
struct fmt_list : virtual z80_opcode_fmt
{
    static inline z80_opc_list opc; 
    virtual z80_stmt_opcode& get_opc() const override 
    {
        return opc;
    }
};

// branch opcode format
struct fmt_branch : virtual z80_opcode_fmt
{
    static inline z80_opc_branch opc; 
    virtual z80_stmt_opcode& get_opc() const override 
    {
        return opc;
    }
};

// dbcc opcode format
struct fmt_dbcc : virtual z80_opcode_fmt
{
    static inline z80_opc_general opc; 
    virtual z80_stmt_opcode& get_opc() const override 
    {
        return opc;
    }
};

}

#endif
