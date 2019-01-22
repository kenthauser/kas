#ifndef KAS_Z80_OPC_LIST_H
#define KAS_Z80_OPC_LIST_H

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


//#include "z80_arg.h"

//#include "z80_insn_eval.h"
//#include "z80_formats_type.h"
//#include "z80_insn_serialize.h"
//#include "z80_insn_validate.h"
//#include "z80_insn_impl.h"

//#include "target/tgt_insn_serialize.h"
#include "tgt_opcode.h"

//#include "kas_core/opcode.h"
//#include "kas_core/core_print.h"
//#include "parser/stmt_print.h"

//#include "z80_opcode_emit.h"

//#include "z80/z80_stmt_opcode.h"
//#include "z80_opcode_emit.h"
//#include "target/tgt_insn_eval.h"

namespace kas::tgt::opc
{

template <typename MCODE_T>
struct tgt_opc_list : tgt_opcode<MCODE_T>
{
    using base_t  = tgt_opcode<MCODE_T>;
    using mcode_t = MCODE_T;
   
    // XXX don't know why base_t types aren't found.
    // XXX expose types & research later
    using insn_t       = typename base_t::insn_t;
    using bitset_t     = typename base_t::bitset_t;
    using arg_t        = typename base_t::arg_t;
    using stmt_args_t  = typename base_t::stmt_args_t;
    using mcode_size_t = typename base_t::mcode_size_t;
    using op_size_t    = typename base_t::op_size_t;

    // XXX also need to expose `base_t` inherited types
    using Inserter     = typename base_t::Inserter;
    using fixed_t      = typename base_t::fixed_t;
    using Iter         = typename base_t::Iter;

    OPC_INDEX();
    const char *name() const override { return "TGT_LIST"; }

    core::opcode& gen_insn(
                 // results of "validate" 
                   insn_t const&  insn
                 , bitset_t&      ok
                 , mcode_t const *mcode_p
                 , stmt_args_t&&  args

                 // and kas_core boilerplate
                 , Inserter&  di
                 , fixed_t&   fixed
                 , op_size_t& insn_size
                 ) override
    {
        // process insn for size before saving
        eval_insn_list(insn, ok, args, insn_size, expression::expr_fits(), this->trace);

        // serialize format (for unresolved instructions)
        // 0) fixed area: OK bitset in host order
        // 1) insn index
        // 2) dummy zero base opcode (word)
        // 3) serialized args

        auto inserter = tgt_data_inserter<typename mcode_t::mcode_size_t>(di, fixed);
        inserter.reserve(-1);       // skip fixed area
        
        inserter(insn.index);
        auto& mc = *insn.list_mcode_p;
        tgt_insert_args(inserter, mc, std::move(args));
        
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

        bitset_t ok(this->fixed_p->fixed);

        auto  reader = tgt_data_reader<typename mcode_t::mcode_size_t>(it, *this->fixed_p, cnt);
        reader.reserve(-1);

        auto& insn = insn_t::get(reader.get_fixed(sizeof(insn_t::index)));
        //auto  data_p = reader.get_fixed_p(M_SIZE_WORD);
        auto& mc = *insn.list_mcode_p;
        auto  args = serial_args(reader, mc);

        // print OK bits & name...
        os << ok.to_string().substr(ok.size() - insn.mcodes.size()) << " " << insn.name;

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
        if (this->trace) *this->trace << std::endl;
        
        // deserialize insn data
        // format:
        //  0) fixed area: OK bitset in host order
        //  1) insn index
        //  2) dummy word to hold args
        //  3) serialized args

        bitset_t ok(this->fixed_p->fixed);

        auto  reader = tgt_data_reader<typename mcode_t::mcode_size_t>(it, *this->fixed_p, cnt);
        reader.reserve(-1);

        auto& insn = insn_t::get(reader.get_fixed(sizeof(insn_t::index)));
        
        auto& mc = *insn.list_mcode_p;
        auto  args = serial_args(reader, mc);

        // evaluate with new `fits`
        eval_insn_list(insn, ok, args, *this->size_p, fits, this->trace);
        
        // save new "OK"
        this->fixed_p->fixed = ok.to_ulong();
        return *this->size_p;
    }

    void emit(Iter it, uint16_t cnt, core::emit_base& base, core::core_expr_dot const *dot_p) override
    {
        bitset_t ok(this->fixed_p->fixed);

        auto  reader = tgt_data_reader<typename mcode_t::mcode_size_t>(it, *this->fixed_p, cnt);
        reader.reserve(-1);

        auto& insn    = insn_t::get(reader.get_fixed(sizeof(insn_t::index)));
        auto& list_mc = *insn.list_mcode_p;
        auto  args    = serial_args(reader, list_mc);

        // get best match
        op_size_t size; 

        // XXX need core_fits 
        auto mcode_p = eval_insn_list(insn, ok, args, size, expression::expr_fits(), this->trace);
        
        // XXX need to handle error case...

        // get opcode "base" value
        auto& mc = *mcode_p;
        auto code = mc.code();

        // Insert args into machine code "base" value
        auto& fmt         = mc.fmt();
        auto& vals        = mc.vals();
        auto val_iter     = vals.begin();
        auto val_iter_end = vals.end();

        // now that have selected machine code match, must be validator for each arg
        unsigned n = 0;
        for (auto& arg : args)
            fmt.insert(n++, code.data(), arg, &*val_iter++);

        // now use common emit
        mc.emit(base, code.data(), args, dot_p);
    }
};
}

#endif
