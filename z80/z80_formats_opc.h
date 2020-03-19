#ifndef KAS_Z80_Z80_FORMAT_OPC_H
#define KAS_Z80_Z80_FORMAT_OPC_H

#include "z80_mcode.h"
#include "z80_opc_branch.h"
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

// use `opc` generic base classes
using fmt_gen  = tgt::opc::tgt_fmt_opc_gen <z80_mcode_t>;
using fmt_list = tgt::opc::tgt_fmt_opc_list<z80_mcode_t>;


// declare Z80 specific opcode formats
using fmt_djnz = fmt_gen;

// branch opcode format
struct fmt_jr : virtual z80_mcode_t::fmt_t
{
    virtual opcode_t& get_opc() const override 
    {
        static z80_opc_branch opc; 
        return opc;
    }
};

}

#endif
