#ifndef KAS_M68K_M68K_FORMATS_OPC_H
#define KAS_M68K_M68K_FORMATS_OPC_H

#include "m68k_mcode.h"
#include "target/tgt_format.h"

#if 0
#include "opc_branch.h"
#include "opc_dbcc.h"
#include "opc_cas2.h"

#include "opc_cp_branch.h"
#include "opc_cp_dbcc.h"
#endif

#include "m68k_opc_branch.h"


namespace kas::m68k::opc
{
// import types needed for `calc_size`
using opcode_data = core::opcode_data;
using core_fits   = core::core_fits;


// declare M68K opcodes

// opcode mixin type to support coldfire 3-word opcode limit
template <typename BASE_T>
struct opc_limit3w : BASE_T
{
    using base_t      = BASE_T;
   
    // coldfire: data size can't exceed 3 words
    op_size_t calc_size(opcode_data& data, core_fits const& fits) const override
    {
        base_t::calc_size(data, fits);
        if (!hw::cpu_defs[hw::limit_3w{}])
            if (data.size.min > 6)
            {
                data.size = 6;
                data.set_error(hw::limit_3w::name::value);
            }
        
        return data.size;
    }
};

// m68k_opcodes: interpose `limit_3w` machinery into `tgt_opc_*` processing
struct m68k_opc_general : opc_limit3w<tgt::opc::tgt_opc_general<m68k_mcode_t>>
{
    OPC_INDEX();
};


// declare M68K formatter base types
// override `tgt_fmt_*` to use m68k opcodes
struct fmt_gen : tgt::opc::tgt_fmt_opc_gen<m68k_mcode_t>
{
    virtual opcode_t& get_opc() const override
    {
        static m68k_opc_general opc;
        return opc;
    }
};


using fmt_list   = tgt::opc::tgt_fmt_opc_list  <m68k_mcode_t>;
using fmt_branch = tgt::opc::tgt_fmt_opc_branch<m68k_mcode_t>;

//using fmt_branch    = fmt_gen;
using fmt_dbcc      = fmt_gen;
using fmt_cas2      = fmt_gen;
using fmt_cp_branch = fmt_gen;
using fmt_cp_dbcc   = fmt_gen;


#if 0
// branch opcode format
struct fmt_branch : tgt::opc::tgt_fmt_opc_branch<m68k_mcode_t>
{
};
#endif

#if 1
#else
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
#endif
}

#endif
