#ifndef KAS_M68K_M68K_FORMATS_OPC_H
#define KAS_M68K_M68K_FORMATS_OPC_H

#include "m68k_mcode.h"
#include "target/tgt_format.h"

#include "opc_branch.h"
#include "opc_dbcc.h"
#include "opc_cas2.h"

#include "opc_cp_branch.h"
#include "opc_cp_dbcc.h"



namespace kas::m68k::opc
{

// get `opc` generic base classes
using fmt_gen  = tgt::opc::tgt_fmt_opc_gen <m68k_mcode_t>;
using fmt_list = tgt::opc::tgt_fmt_opc_list<m68k_mcode_t>;


// branch opcode format
struct fmt_branch : virtual m68k_mcode::fmt_t
{
    virtual opcode_t& get_opc() const override 
    {
        static m68k_opc_branch opc; 
        return opc;
    }
};

// dbcc opcode format
struct fmt_dbcc : virtual m68k_mcode::fmt_t
{
    virtual opcode_t& get_opc() const override 
    {
        static m68k_opc_general opc; 
        return opc;
    }
};

// special for CAS2 
struct fmt_cas2 : virtual fmt_gen
{
    virtual opcode_t& get_opc() const override 
    {
        static m68k_opc_cas2 opc; 
        return opc;
    }
};

// co-processor branch opcode format
struct fmt_cp_branch : virtual m68k_mcode::fmt_t
{
    virtual opcode_t& get_opc() const override 
    {
        static m68k_opc_cp_branch opc; 
        return opc;
    }
};

// co-processor dbcc opcode format
struct fmt_cp_dbcc : virtual m68k_mcode::fmt_t
{

    virtual opcode_t& get_opc() const override 
    {
        static m68k_opc_cp_dbcc opc; 
        return opc;
    }
};
}

#endif
