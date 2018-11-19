#ifndef KAS_M68K_STMT_IMPL_H
#define KAS_M68K_STMT_IMPL_H

#include "m68k_stmt.h"

// args is a container of "m68k_arg_t" from comma-separated arguments
//
// Two special caes:
//  1)  if opcode has no arguments (eg "nop"), a single "MODE_NONE"
//      is created for op-code level error messages
//  2)  bitfield arguments (the offset, width portion) are not separated
//      by commas, but are a separate "m68k_arg_t" instance.
namespace kas::m68k
{

using namespace kas::core::opc;

opcode& stmt_m68k::gen_insn(core::opcode::Inserter& di
                          , core::opcode::fixed_t& fixed
                          , core::opcode::op_size_t& insn_size)
{
    // get support types from INSN
    using insn_bitset_t = typename insn_t::insn_bitset_t;
    using opcode_t      = typename insn_t::opcode_t;
    using err_msg_t     = typename opcode_t::err_msg_t;

    auto trace = opcode::trace;
    auto& insn = *insn_p;

    // generate an "error" opcode if appropriate
    auto make_error = [&fixed=fixed,&insn_size=insn_size, &trace]
            (parser::tagged_msg const& msg) -> opcode&
        {
            static opc_error err;
            
            if (trace)
                *trace << "ERR: " << msg.msg << std::endl;
            
            fixed.diag = kas::parser::kas_diag::error(msg).ref();
            insn_size.set_error();
            return err;
        };

    // copy "opcode" location from first (possibly dummy arg)
    parser::kas_position_tagged opc_pos = args.front();
    
    // if first is dummy, clear args
    if (args.front().is_missing())
        args.clear();

    // print name/args
    if (trace)
    {
        *trace << "gen_insn: " << insn.name() << " [" << insn.opcodes.size() << " opcodes]";
        for (auto& arg : args)
            *trace << ", " << arg;
        *trace << std::endl;
    }

    // validate arg & addressing mode for each arg
    // also note if all args are "const" (ie: just regs & literals)
    bool args_are_const = true;
    if (auto diag = insn.validate_args(opc_pos, args, args_are_const, trace))
        return make_error(diag);

    // select opcode matching args (normally only 1)
    insn_bitset_t ok;
    opcode_t const* matching_opcode_p {};
    bool multiple_matches = false;
    const char *first_msg{};

    // loop thru opcodes, recording first error & recording all matches
    int i = 0; 
    for (auto& op_p : insn.opcodes)
    {
        if (trace)
            *trace << "validating: " << +i << ": ";

        auto diag = op_p->validate_args(args, trace);
        
        if (trace)
            *trace << (diag ? diag : " = OK") << std::endl;

        if (!diag) {
            ok.set(i);
            if (!matching_opcode_p)
                matching_opcode_p = op_p;
            else
                multiple_matches = true;
        } else if (!first_msg)
            first_msg = diag;
        
        ++i;        // next
    }
    
    if (trace)
        *trace << "result: " << ok.count() << " OK" << std::endl;

    // no match: Invalid arguments
    if (!matching_opcode_p)
        return make_error({error_msg::ERR_invalid, opc_pos});

    // multiple match means no match
    if (multiple_matches)
        matching_opcode_p = {};


    // logic: here at least one arg matches. 
    // 1) if constant `args`, emit binary code
    // 2) if single match, use format for selected opcode
    // 3) otherwise, use opcode for "list"
#if 0
    if (0 && args_are_const)
    {
        // all const args: can select best opcode & calculate size
        if (!matching_opcode_p)
        {
            matching_opcode_p = eval_insn_list(
                                    insn, ok, args
                                  , insn_size, expression::expr_fits(), trace);
        }

        // single opcode matched: calculate size to select format
        else
        {
            insn_size = sizeof(uint16_t) * (1 + matching_opcode_p->opc_long);
            for (auto& arg : args)
                insn_size += arg.size(expression::expr_fits{});
        }

        if (trace)
            *trace << "size = " << insn_size << std::endl;
#if 0
        // if binary data fits in "fixed" area of opcode, just emit as binary data
        if (insn_size() <= m68k_opc_resolved::max_size)
        {
            m68k_opc_resolved opc;
            return opc.gen_insn(
                          insn
                        , ok
                        , matching_opcode_p
                        , std::move(args)
                        , di, fixed, insn_size
                        );
        }
#endif
    }
#endif

    // select "fmt" from `opcode_p` if set, else "list"
    auto& fmt = matching_opcode_p ? matching_opcode_p->fmt()
                                  : opcode_t::fmt_t::get_list_fmt()
                                  ;

    // now use "format" to generate opcode
    return fmt.get_opc().gen_insn(
              insn
            , ok
            , matching_opcode_p
            , std::move(args)
            
            // and boilerplate
            , di, fixed, insn_size
            );
}
}

#endif
