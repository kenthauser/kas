#ifndef KAS_Z80_Z80_OPC_DBCC_H
#define KAS_Z80_Z80_OPC_DBCC_H


//
// dbCC Dn, <label>
//
// <label> displacement limited to 16-bit displacemnt. If out of range, emit as if the
// following was coded:
//
//      dbCC dn, 1f     | size = 4
//      bra  2f         | size = 2
// 1:   jmp  <label>    | size = 6
// 2:                   | total = 12


#include "z80_stmt_opcode.h"
#include "z80_opcode_emit.h"

namespace kas::z80::opc
{

struct z80_opc_dbcc: z80_stmt_opcode
{
    using base_t::base_t;


    OPC_INDEX();
    const char *name() const override { return "Z80_DBCC"; }

    core::opcode& gen_insn(
                 // results of "validate" 
                   z80_insn_t   const&        insn
                 , z80_insn_t::insn_bitset_t& ok
                 , z80_opcode_t const        *opcode_p
                 , ARGS_T&&                    args
                 // and kas_core boilerplate
                 , Inserter& di
                 , fixed_t& fixed
                 , op_size_t& insn_size
                 ) override
    {
        // get opcode & arg info
        auto& op   = *opcode_p;
        auto arg_p = args.begin();
        insn_size  = {4, 12};          // include pseudo-code if branch distance exceeded

        // save "index", reg_num, and dest (as expression)
        auto inserter = z80_data_inserter(di, fixed);
        
        inserter(op.index              , M_SIZE_WORD);  // save opcode "index"
        inserter(std::move(arg_p->expr), M_SIZE_AUTO);  // save loop destination
        return *this;
    }
    
    void fmt(Iter it, uint16_t cnt, std::ostream& os) override
    {
        // deserialize insn data
        // format:
        //  1) opcode index
        //  2) data reg #
        //  3) destination as expression

        auto  reader  = z80_data_reader(it, *fixed_p, cnt);
        auto& opcode  = z80_opcode_t::get(reader.get_fixed(M_SIZE_WORD));
        auto& dest    = reader.get_expr();
        
        // print "name" & "size"...
        os << opcode.defn().name();
        
        // ...print opcode...
        os << std::hex << " " << std::setw(4) << opcode.code();

        // ...and arg
        os << dest;
    }

    op_size_t calc_size(Iter it, uint16_t cnt, core::core_fits const& fits) override
    {
        // just get destination
        auto& dest = *it;
        
        // check for word offset
        switch(fits.disp<int16_t>(dest, 2))
        {
            case expression::NO_FIT:
                return size_p->max;     // doesn't fit in range
            case expression::DOES_FIT:
                return size_p->min;     // does fit in range
            default:
                return *size_p;         // unknown: no change
        }
    }

    void emit(Iter it, uint16_t cnt, core::emit_base& base, core::core_expr_dot const *dot_p) override
    {
        auto  reader  = z80_data_reader(it, *fixed_p, cnt);
        auto& opcode  = z80_opcode_t::get(reader.get_fixed(M_SIZE_WORD));
        auto& dest    = reader.get_expr();
       
        // add "register" into DBcc base opcode
        uint16_t code = opcode.code();

        // if "fits" just do word displacement
        if (size_p->max == 4)
        {
            base << code << core::emit_disp(*dot_p, 2, 2) << dest;
                
        } else
        {
            base << code << core::set_size(2) << 2;     // 4: DBcc: jump over branch
            base << core::set_size(2) << 0x6006;        // 2: BRA: jump over jump, displacement = 6
            base << core::set_size(2) << 0x4ef9;        // 2: JMP ( ).L
            base << core::set_size(4) << dest;          // 4: relocatable address
        }
    }
};
}

#endif
