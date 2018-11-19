#ifndef KAS_M68K_M68K_FORMATS_OPC_H
#define KAS_M68K_M68K_FORMATS_OPC_H

#include "m68k_formats_type.h"

#include "opc_general.h"
#include "opc_list.h"
#include "opc_branch.h"
#include "opc_dbcc.h"
#include "opc_cas2.h"

#include "opc_cp_branch.h"
#include "opc_cp_dbcc.h"

namespace kas::m68k::opc
{

// general opcode format
struct fmt_gen : virtual m68k_opcode_fmt
{
    static inline m68k_opc_general opc; 
    virtual m68k_stmt_opcode& get_opc() const override 
    {
        return opc;
    }
};

// list opcode format
struct fmt_list : virtual m68k_opcode_fmt
{
    static inline m68k_opc_list opc; 
    virtual m68k_stmt_opcode& get_opc() const override 
    {
        return opc;
    }
};

// branch opcode format
struct fmt_branch : virtual m68k_opcode_fmt
{
    static inline m68k_opc_branch opc; 
    virtual m68k_stmt_opcode& get_opc() const override 
    {
        return opc;
    }
};

// dbcc opcode format
struct fmt_dbcc : virtual m68k_opcode_fmt
{
    static inline m68k_opc_general opc; 
    virtual m68k_stmt_opcode& get_opc() const override 
    {
        return opc;
    }
};

// special for CAS2 
struct fmt_cas2 : virtual fmt_gen
{
    static inline m68k_opc_cas2 opc; 
    virtual m68k_stmt_opcode& get_opc() const override 
    {
        return opc;
    }
};

// co-processor branch opcode format
struct fmt_cp_branch : virtual m68k_opcode_fmt
{
    static inline m68k_opc_cp_branch opc; 
    virtual m68k_stmt_opcode& get_opc() const override 
    {
        return opc;
    }
};

// co-processor dbcc opcode format
struct fmt_cp_dbcc : virtual m68k_opcode_fmt
{
    static inline m68k_opc_cp_dbcc opc; 

    virtual m68k_stmt_opcode& get_opc() const override 
    {
        return opc;
    }
};
}

#endif
