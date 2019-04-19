#ifndef KAS_Z80_Z80_FORMAT_OPC_H
#define KAS_Z80_Z80_FORMAT_OPC_H

#include "z80_mcode.h"
#include "target/tgt_format.h"

namespace kas::z80::opc
{
// derive z80 version of `tgt` classes
// use generic bit inserter/extractor
template <unsigned...Ts>
using fmt_generic = tgt::opc::tgt_fmt_generic<z80_mcode_t, Ts...>;

// use generic template to generate `mix-in` type
template <unsigned N, typename T>
using fmt_arg = tgt::opc::tgt_fmt_arg<z80_mcode_t, N, T>;

// get `opc` generic base classes
using fmt_gen  = tgt::opc::tgt_fmt_opc_gen <z80_mcode_t>;
using fmt_list = tgt::opc::tgt_fmt_opc_list<z80_mcode_t>;

using fmt_jr   = fmt_gen;
using fmt_djnz = fmt_gen;

#if 0
// branch opcode format
struct fmt_branch : virtual z80_mcode::fmt_t
{
    virtual opcode_t& get_opc() const override 
    {
        static z80_opc_branch opc; 
        return opc;
    }
};

// dbcc opcode format
struct fmt_dbcc : virtual z80_mcode::fmt_t
{
    virtual opcode_t& get_opc() const override 
    {
        static z80_opc_general opc; 
        return opc;
    }
};

// special for CAS2 
struct fmt_cas2 : virtual fmt_gen
{
    virtual opcode_t& get_opc() const override 
    {
        static z80_opc_cas2 opc; 
        return opc;
    }
};

// co-processor branch opcode format
struct fmt_cp_branch : virtual z80_mcode::fmt_t
{
    virtual opcode_t& get_opc() const override 
    {
        static z80_opc_cp_branch opc; 
        return opc;
    }
};

// co-processor dbcc opcode format
struct fmt_cp_dbcc : virtual z80_mcode::fmt_t
{

    virtual opcode_t& get_opc() const override 
    {
        static z80_opc_cp_dbcc opc; 
        return opc;
    }
};
#endif
}

#endif
