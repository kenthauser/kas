#ifndef KAS_M68K_OPC_LIST_H
#define KAS_M68K_OPC_LIST_H

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


#include "m68k_arg_defn.h"
#include "m68k_arg_size.h"

#include "m68k_insn_eval.h"
#include "m68k_formats_type.h"
#include "m68k_insn_serialize.h"
#include "m68k_insn_validate.h"
//#include "m68k_insn_impl.h"


#include "kas_core/opcode.h"
#include "kas_core/core_print.h"
//#include "parser/stmt_print.h"

#include "m68k_opcode_emit.h"

#include "m68k_stmt_opcode.h"
#include "m68k_opcode_emit.h"


namespace kas::m68k::opc
{
struct m68k_opc_list: m68k_stmt_opcode
{
    using base_t::base_t;

    static auto& fmt()
    {
        // break include loop
        static const auto& _fmt = m68k_opcode_fmt::get_list_fmt();
        return _fmt;
    }

    OPC_INDEX();
    const char *name() const override { return "M68K_LIST"; }

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
        // process insn for size before saving
        eval_insn_list(insn, ok, args, insn_size, expression::expr_fits(), trace);

        // serialize format (for unresolved instructions)
        // 0) fixed area: OK bitset in host order
        // 1) insn index
        // 2) dummy zero base opcode (word)
        // 3) serialized args

        auto inserter = m68k_data_inserter(di, fixed);
        inserter.reserve(-1);       // skip fixed area
        
        inserter(insn.index, M_SIZE_WORD);
        auto data_p = inserter(0, M_SIZE_WORD);
        m68k_insert_args(inserter, std::move(args), data_p, fmt());
        
        // store OK bitset in fixed area
        fixed.fixed = ok.to_ulong();
        return *this;
    }

    void fmt(Iter it, uint16_t cnt, std::ostream& os) override
    {
        // deserialize insn data
        // format:
        //  0) fixed area: OK bitset in host order
        //  1) insn index
        //  2) dummy word to hold args
        //  3) serialized args

        m68k_insn_t::insn_bitset_t ok(fixed_p->fixed);

        auto  reader = m68k_data_reader(it, *fixed_p, cnt);
        reader.reserve(-1);

        auto& insn = m68k_insn_t::get(reader.get_fixed(M_SIZE_WORD));
        auto  op_p = reader.get_fixed_p(M_SIZE_WORD);
        auto  args = serial_args{reader, fmt(), op_p};

        // print OK bits & name...
        os << ok.to_string().substr(ok.size() - insn.opcodes.size()) << " " << insn.name();

        // ...and args
        auto delim = " : ";
        for (auto& arg : args)
        {
            os << delim << arg;
            delim = ",";
        }
    }

    op_size_t calc_size(Iter it, uint16_t cnt, core::core_fits const& fits) override
    {
        if (trace) *trace << std::endl;
        
        // deserialize insn data
        // format:
        //  0) fixed area: OK bitset in host order
        //  1) insn index
        //  2) dummy word to hold args
        //  3) serialized args

        m68k_insn_t::insn_bitset_t ok(fixed_p->fixed);

        auto  reader = m68k_data_reader(it, *fixed_p, cnt);
        reader.reserve(-1);

        auto& insn = m68k_insn_t::get(reader.get_fixed(M_SIZE_WORD));
        auto  op_p = reader.get_fixed_p(M_SIZE_WORD);
        auto  args = serial_args{reader, fmt(), op_p};

        // evaluate with new `fits`
        eval_insn_list(insn, ok, args, *size_p, fits, trace);
        
        // save new "OK"
        fixed_p->fixed = ok.to_ulong();
        return *size_p;
    }

    void emit(Iter it, uint16_t cnt, core::emit_base& base, core::core_expr_dot const& dot) override
    {
        m68k_insn_t::insn_bitset_t ok(fixed_p->fixed);

        auto  reader = m68k_data_reader(it, *fixed_p, cnt);
        reader.reserve(-1);

        auto& insn = m68k_insn_t::get(reader.get_fixed(M_SIZE_WORD));
        auto  op_p = reader.get_fixed_p(M_SIZE_WORD);
        auto  args = serial_args{reader, fmt(), op_p};

        // get best match
        op_size_t size; 
        auto opcode_p = eval_insn_list(insn, ok, args, size, expression::expr_fits(), trace);
        
        // XXX need to handle error case...

        // get opcode "base" value
        auto& opcode = *opcode_p;
        uint16_t code[2];
        if (opcode.opc_long)
        {
            code[0] = opcode.code() >> 16;
            code[1] = opcode.code();
        } else
        {
            code[0] = opcode.code();
        }

        // Insert args into opcode "base" value
        auto& fmt = opcode.fmt();
        unsigned n = 0;
        for (auto& arg : args)
            fmt.insert(n++, code, arg);

        // now use common emit
        opcode.emit(base, code, args, dot);
    }
};
}

#endif
