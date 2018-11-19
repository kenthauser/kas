#ifndef KAS_M68K_M68K_OPC_CAS2_H
#define KAS_M68K_M68K_OPC_CAS2_H


//
// CAS2 is the only "3-word" opcode.
//
// Accumulate register-pair args in "zero" base-code 32-bit word.
// Emit opcode base code & register data
//

#include "m68k_stmt_opcode.h"
#include "m68k_opcode_emit.h"

namespace kas::m68k::opc
{

struct m68k_opc_cas2: m68k_stmt_opcode
{
    using base_t::base_t;


    OPC_INDEX();
    const char *name() const override { return "M68K_CAS2"; };

    core::opcode& gen_insn(
                 // results of "validate" 
                   m68k_insn_t   const&        insn
                 , m68k_insn_t::insn_bitset_t& ok
                 , m68k_opcode_t const        *opcode_p
                 , ARGS_T&&                    args
                 // and kas_core boilerplate
                 , Inserter& di
                 , fixed_t& fixed
                 , op_size_t& insn_size
                 ) override
    {
        // get opcode & arg info
        auto& op   = *opcode_p;
        insn_size  = 6;         // always 3 words

        // serialize format
        // 1) opcode index
        // 2) zero-word base code for args
        // 3) serialized args

        auto inserter = m68k_data_inserter(di, fixed);
        
        inserter(op.index, M_SIZE_WORD);            // save opcode "index"
        auto data_p = inserter(0, M_SIZE_LONG);     // zero base for extension words
        m68k_insert_args(inserter, std::move(args), data_p, op.fmt());
        return *this;
    }
    
    void fmt(Iter it, uint16_t cnt, std::ostream& os) override
    {
        // deserialize insn data
        // format:
        //  1) opcode index
        //  2) opcode binary code (word or long)
        //  3) serialized args
        auto  reader = m68k_data_reader(it, *fixed_p, cnt);
        auto& opcode = m68k_opcode_t::get(reader.get_fixed(M_SIZE_WORD));
        auto  op_p   = reader.get_fixed_p(M_SIZE_LONG);
        auto  args   = serial_args(reader, opcode.fmt(), op_p);

        // print "name" & "size"...
        os << opcode.defn().name() << ":";
        os << m68k_insn_size::m68k_size_suffixes[opcode.insn_sz]; 
        
        // ...print opcode...
        os << std::hex << " " << std::setw(2) << opcode.code();

        // ...and args
        auto delim = " : ";
        for (auto& arg : args)
        {
            os << delim << arg;
            delim = ",";
        }
    }

    void emit(Iter it, uint16_t cnt, core::emit_base& base, core::core_expr_dot const& dot) override
    {
        // deserialize insn data
        // format:
        //  1) opcode index
        //  2) opcode binary code (word or long)
        //  3) serialized args (unneeded)
        auto  reader = m68k_data_reader(it, *fixed_p, cnt);
        auto& opcode = m68k_opcode_t::get(reader.get_fixed(M_SIZE_WORD));
        auto  op_p   = reader.get_fixed_p(M_SIZE_LONG);

        // output 3 words
        uint16_t code = opcode.code();
        base << code << op_p[0] << op_p[1];
    }
};
}

#endif
