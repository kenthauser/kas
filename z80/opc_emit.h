#ifndef KAS_Z80_OPC_EMIT_H
#define KAS_Z80_OPC_EMIT_H

///////////////////////////////////////////////////////////////////////////
//
//                  m 6 8 k _ o p c o d e s
//
///////////////////////////////////////////////////////////////////////////
//
// The opcodes class provides the interface between the parser and the
// object-code generation modules of the assembler.
//
// The specialized `opcodes` for z80 instruction set facitate a method
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
// A final opcode format (`z80_list_opcode`) is used to hold an
// unresolved opcode & argument list  when the matching instruction can
// not be determined by initial inspection of the arguments. A good
// example of this is `general move` vs `moveq` when immediate argument
// is an expression.
//
///////////////////////////////////////////////////////////////////////////


//#include "opc_common.h"

namespace kas { namespace z80 { namespace opc
{

    struct z80_opc_emit : opcode
    {
        OPC_INDEX();
        const char *name() const override { return "Z80_EMIT"; }

        z80_opc_emit() = default;

        template <typename Insn, typename Z80_Inserter>
        z80_opc_emit(Insn const& insn, Z80_Inserter& inserter
                       , op_size_t const& size
                       , args_t&& args)
        {
            auto info = insn.info();
            auto opcode = &insn.opcode[0];

            // save location of first opcode
            auto op_p = inserter(*opcode++, M_SIZE_WORD);

            // NB: predecrement because first already inserted
            for (int n = info.opc_cnt; --n;)
                inserter(*opcode++, M_SIZE_WORD);

            // really need arg emit, but works for now
            z80_insert_args(inserter, std::move(args), op_p, insn.fmt_idx, false);

            // update accounting & done...
            *this->size_p = size;
        }

        void fmt(Iter it, uint16_t cnt, std::ostream& os) override
        {
            os << std::hex;

            auto reader = z80_data_reader(it, *fixed_p, cnt);

            int n = size_p->min >> 1;

            while (n--)
                os << ' ' << reader.get_fixed(M_SIZE_WORD);
            os << std::endl;
        }

        void emit(Iter it, uint16_t cnt, core::emit_base& base, core::core_expr_dot const *) override
        {
            auto reader = z80_data_reader(it, *fixed_p, cnt);

            int n = size_p->min >> 1;
            while (n--)
                base << (uint16_t)reader.get_fixed(M_SIZE_WORD);
        }
    };
}}}

#endif
