#ifndef KAS_M68K_OPC_DBCC_H
#define KAS_M68K_OPC_DBCC_H


//
// DBcc Dn, <label>
//
// <label> displacement limited to 16-bit displacemnt. If out of range,
// emit as if the following was coded:
//
//      DBcc dn, 1f     | base_size = 4 (or 6+ for cpDBcc)
//      bra  2f         | size = 2
// 1:   bra  <label>    | size = 6
// 2:                   | total = 12 (or 14+ for cpDBcc)
//
// NB: if `bra.l` not supported, a "jmp.l" insn is used

#include "m68k_mcode.h"
#include "expr/expr_fits.h"
#include "target/tgt_format.h"

namespace kas::m68k::opc
{

// define `opc_dbcc` "opcode" as variant of branch.
struct m68k_opc_dbcc : tgt::opc::tgt_opc_branch<m68k_mcode_t>
{
    using base_t = tgt::opc::tgt_opc_branch<m68k_mcode_t>;

    OPC_INDEX();
    using NAME = KAS_STRING("M68K_DBCC");
    using expr_fits = typename expression::expr_fits;
    
    void do_calc_size(data_t&                data
                    , mcode_t const&         mcode
                    , expr_t const&          dest
                    , stmt_info_t&           info
                    , unsigned               mode
                    , expr_fits const& fits) const override
    {
        // calculate displacement as if branch
        base_t::do_calc_size(data, mcode, dest, info, mode, fits);

        // disallow deletion.
        auto dbcc_size = mcode.base_size() + 2; // base plus word offset

        if (data.size.min < dbcc_size)
            data.size.min = dbcc_size;      
        // "long" branch is very long
        if (data.size.max > dbcc_size)
            data.size.max = dbcc_size + 8;
    }

    void do_emit     (data_t const&          data
                    , core::core_emit&       base
                    , mcode_t const&         mcode
                    , mcode_size_t          *code_p
                    , expr_t const&          dest
                    , unsigned               arg_mode) const override
    {
        // if "short" version, use base_t emitter
        auto base_size = mcode.base_size() + 2; // base plus word offset
        
        if (data.size() == base_size)
            return base_t::do_emit(data, base, mcode, code_p, dest, arg_mode);

        if (data.size() != base_size + 8)
            throw std::runtime_error{std::string(NAME()) + "invalid size: "
                                     + std::to_string(data.size())};

        // emit cpDBcc opcode + boilerplate
        auto words = mcode.code_size()/sizeof(mcode_size_t);
        while (words--)
            base << *code_p++;

        base << (uint16_t) 2;       // branch offset
        base << (uint16_t) 0x6006;  // bra 2f
        base << (uint16_t) 0x4ef9;  // jmp (xxx).L opcode
        base << core::set_size(4) << dest; // address
    }
};

struct fmt_dbcc : virtual m68k_mcode_t::fmt_t
{
    opcode_t& get_opc() const override
    {
        static m68k_opc_dbcc opc;
        return opc;
    }
};

}

#endif

