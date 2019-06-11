#ifndef KAS_ARM_ARM_FORMAT_OPC_H
#define KAS_ARM_ARM_FORMAT_OPC_H

#include "arm_mcode.h"
#include "target/tgt_format.h"

namespace kas::arm::opc
{
// derive arm version of `tgt` classes
// get `opc` generic base classes
using fmt_gen  = tgt::opc::tgt_fmt_opc_gen <arm_mcode_t>;
using fmt_list = tgt::opc::tgt_fmt_opc_list<arm_mcode_t>;

using fmt_jr   = fmt_gen;
using fmt_djnz = fmt_gen;

#if 0
// branch opcode format
struct fmt_branch : virtual arm_mcode::fmt_t
{
    virtual opcode_t& get_opc() const override 
    {
        static arm_opc_branch opc; 
        return opc;
    }
};

// dbcc opcode format
struct fmt_dbcc : virtual arm_mcode::fmt_t
{
    virtual opcode_t& get_opc() const override 
    {
        static arm_opc_general opc; 
        return opc;
    }
};

// special for CAS2 
struct fmt_cas2 : virtual fmt_gen
{
    virtual opcode_t& get_opc() const override 
    {
        static arm_opc_cas2 opc; 
        return opc;
    }
};

// co-processor branch opcode format
struct fmt_cp_branch : virtual arm_mcode::fmt_t
{
    virtual opcode_t& get_opc() const override 
    {
        static arm_opc_cp_branch opc; 
        return opc;
    }
};

// co-processor dbcc opcode format
struct fmt_cp_dbcc : virtual arm_mcode::fmt_t
{

    virtual opcode_t& get_opc() const override 
    {
        static arm_opc_cp_dbcc opc; 
        return opc;
    }
};
#endif
}

#endif
