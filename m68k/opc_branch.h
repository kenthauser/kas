#ifndef KAS_M68K_OPC_BRANCH_H
#define KAS_M68K_OPC_BRANCH_H

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


//#include "opc_common.h"
#include "m68k_stmt_opcode.h"
#include "m68k_opcode_emit.h"

namespace kas::m68k::opc
{

struct m68k_opc_branch : m68k_stmt_opcode
{
    using base_t::base_t;


    OPC_INDEX();
    const char *name() const override { return "M68K_BRANCH"; }

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
        insn_size = {0, 6};     // ranges from deleted to long branch/jmp

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
        op_size_t new_size;

        // check for zero offset
        if (this->size_p->min == 0) {
            if (fits.seen_this_pass(dest)) {
                new_size = 2;
            } else {
                switch(fits.zero(dest, 0))
                {
                    case expression::DOES_FIT:
                        return 0;
                    case expression::NO_FIT:
                        new_size = 2;
                        break;
                    default:
                        // allow zero size to be considered
                        break;
                }
            }
        }

        // check for byte offset
        if (this->size_p->min <= 2)
            switch(fits.disp<int8_t>(dest, 2))
            {
                case expression::DOES_FIT:
                    return {new_size.min, 2};
                case expression::NO_FIT:
                    new_size = 4;
                    break;
                default:
                    break;
            }

        // check for word offset
        switch(fits.disp<int16_t>(dest, 4))
        {
            case expression::NO_FIT:
                return 6;
            case expression::DOES_FIT:
                new_size.max = 4;
                break;
            default:
                new_size.max = 6;
                break;
        }

        return new_size;
    }

    void emit(Iter it, uint16_t cnt, core::emit_base& base, core::core_expr_dot const& dot) override
    {
        // get opcode & destination 
        auto  reader = m68k_data_reader(it, *fixed_p, cnt);
        auto& opcode = m68k_opcode_t::get(reader.get_fixed(M_SIZE_WORD));
        auto& dest   = reader.get_expr();
        
#if 1
        // XXX long branch not available on 68000
        std::cout << "opc_branch: " << *size_p << " dest = " << dest;
        if (auto sym_p = dest.get_p<core::core_symbol>()) {
            if (auto addr_p = sym_p->addr_p()) {
                if (&addr_p->section() != &dot.section()) {
                    std::cout << " cross-section offset";
                } else {
                    // XXX dot.offset() => absolute offset, not frag offset.
                    auto delta = addr_p->offset() - dot.offset();
                    std::cout << " offset = " << std::hex << std::setw(size_p->max) << delta();
                }
            }
        }
        std::cout << std::endl;
#endif
        uint16_t code = opcode.code();  // base op-code

        switch (size_p->max)
        {
            case 0:
                return;
            case 2:
            {
                // emit insn as two bytes...
                // since DISP is always a constant, can always change to single word output
                base << core::set_size(1) << (code >> 8);
                base << core::emit_disp(dot, 1, 2) << dest;
                break;
            }

            case 4:
                base << code << core::emit_disp(dot, 2, 2) << dest;
                break;
        }
    }
};
}

#endif
