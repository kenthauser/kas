#ifndef KAS_M68K_M68K_FORMATS_OPC_H
#define KAS_M68K_M68K_FORMATS_OPC_H

#include "m68k_mcode.h"
#include "m68k_size_lwb.h"      // defines `m68k_opc_base`

#include "opc_dbcc.h"

#include "target/tgt_format.h"


namespace kas::m68k::opc
{
// derive m68k version of `tgt` classes
// use generic bit inserter/extractor
template <unsigned...Ts>
using fmt_generic = tgt::opc::tgt_fmt_generic<m68k_mcode_t, Ts...>;

// use generic template to generate argument `mix-in` type
template <unsigned N, typename T>
using fmt_arg = tgt::opc::tgt_fmt_arg<m68k_mcode_t, N, T>;


// define opcode mixin type to enforce coldfire 3-word opcode limit
using opcode_data = core::opcode_data;
using core_fits   = core::core_fits;

template <typename BASE_T>
struct opc_limit3w : BASE_T
{
    using base_t    = BASE_T;

    // import types needed for `calc_size`
    using op_size_t   = typename base_t::op_size_t;
  
    // override "constructor" of opcode
    core::opcode *gen_insn(
                 // results of "validate" 
                   typename base_t::insn_t const&  insn
                 , typename base_t::bitset_t&      ok
                 , typename base_t::mcode_t const& mcode
                 , typename base_t::stmt_args_t&&  args
                 , typename base_t::stmt_info_t    stmt_info

                 // and opcode data
                 , opcode_data&  data
                 ) override
    {
        base_t::gen_insn(insn, ok, mcode, std::move(args), stmt_info, data);
        this->cf_limit_3w(data);
        return this;
    }

    // coldfire: data size can't exceed 3 words
    op_size_t calc_size(opcode_data& data, core_fits const& fits) const override
    {     
        base_t::calc_size(data, fits);
        return this->cf_limit_3w(data);
    }
};

// m68k_opcodes: interpose `limit_3w` machinery into `tgt_opc_*` processing
struct m68k_opc_general : opc_limit3w<tgt::opc::tgt_opc_general<m68k_mcode_t>>
{
    OPC_INDEX();
};
struct m68k_opc_list: opc_limit3w<tgt::opc::tgt_opc_list<m68k_mcode_t>>
{
    OPC_INDEX();
};

// declare M68K formatter base types
// override `tgt_fmt_*` to use m68k opcodes (which enforce coldfire 48-bit limit)
struct fmt_gen : tgt::opc::tgt_fmt_opc_gen<m68k_mcode_t>
{
    opcode_t& get_opc() const override
    {
        static m68k_opc_general opc;
        return opc;
    }
};
struct fmt_list: tgt::opc::tgt_fmt_opc_list<m68k_mcode_t>
{
    opcode_t& get_opc() const override
    {
        static m68k_opc_list opc;
        return opc;
    }
};
 
// NB: use `target` default implementation
using fmt_branch = tgt::opc::tgt_fmt_opc_branch<m68k_mcode_t>;
}

#endif
