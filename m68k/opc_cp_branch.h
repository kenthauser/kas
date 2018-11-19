#ifndef KAS_M68K_OPC_CP_BRANCH_H
#define KAS_M68K_OPC_CP_BRANCH_H

///////////////////////////////////////////////////////////////////////////
//
//                  m 6 8 k _ o p c o d e s
//
///////////////////////////////////////////////////////////////////////////
//
// The opcodes class provides the interface between the parser and the
// object-code generation modules of the assembler.
//
// The specialized `opcodes` for m68k instruction set facitate a method
// to pass static information about an instruction to the `size` and
// `emit` functions.  Since, by design, the assembler holds only the
// `opcode` class and the argument info, information to inform
// evalutation of the arguments must be derived from the  `opcode` class.
//
// This information includes the following:
//
// - 16-bit/32-bit instruction
//
// - special branch interpretation
//
// - incorporated immediate argument format (eg: moveq, shift, clr)
//
// - special emit fuction (eg: movem from reg)
//
// The selection of the `opcode` is done via the `op_fn` fuction in the
// instruction `info` class.
//
// A final opcode format (`m68k_list_opcode`) is used to hold an
// unresolved opcode & argument list  when the matching instruction can
// not be determined by initial inspection of the arguments. A good
// example of this is `general move` vs `moveq` when immediate argument
// is an expression.
//
///////////////////////////////////////////////////////////////////////////


#include "m68k_stmt_opcode.h"
#include "m68k_opcode_emit.h"
//#include "opc_common.h"

namespace kas::m68k::opc
{

struct m68k_opc_cp_branch : m68k_stmt_opcode
{
    using base_t::base_t;

    OPC_INDEX();
    const char *name() const override { return "M68K_CP_BR"; }

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
        // get size for this opcode
        auto& op = *opcode_p;
        insn_size = {4, 6};     // can't be deleted: may set exception

        // save "index" and dest (as expression)
        auto inserter = m68k_data_inserter(di, fixed);
        
        inserter(op.index, M_SIZE_WORD);
        inserter(std::move(args.front().disp), M_SIZE_AUTO);
        return *this;
    }

    void fmt(Iter it, uint16_t cnt, std::ostream& os) override
    {
        // deserialize insn data
        // format:
        //  1) opcode index
        //  2) destination as expression

        auto  reader = m68k_data_reader(it, *fixed_p, cnt);
        auto& opcode = m68k_opcode_t::get(reader.get_fixed(M_SIZE_WORD));
        auto& dest   = reader.get_expr();
        
        // print "name"
        os << opcode.defn().name();
        
        // ...print opcode...
        os << std::hex << " " << std::setw(4) << opcode.code();

        // ...and args
        os << " : " << dest;
    }

    op_size_t calc_size(Iter it, uint16_t cnt, core::core_fits const& fits) override
    {
        // don't need to deserialze insn data
        auto& dest = *it;
        
        // check for word offset
        switch(fits.disp<int16_t>(dest, 2))
        {
            case expression::NO_FIT:
                return size_p->max;
            case expression::DOES_FIT:
                return size_p->min;
            default:
                return *size_p;     // no change
        }
    }

    void emit(Iter it, uint16_t cnt, core::emit_base& base, core::core_expr_dot const& dot) override
    {
        // get opcode & destination 
        auto  reader = m68k_data_reader(it, *fixed_p, cnt);
        auto& opcode = m68k_opcode_t::get(reader.get_fixed(M_SIZE_WORD));
        auto& dest   = reader.get_expr();
        
        // flag 16/32 bits in opcode
        uint16_t code = opcode.code();
        if (size_p->max == 6)
            code |= (1 << 6);

        // emit opcode and displacement: CP branches are from "dot + 2"
        base << code << core::emit_disp(dot, size_p->max, 2) << dest;
    }
};
}

#endif
