#ifndef KAS_TARGET_TGT_STMT_IMPL_H
#define KAS_TARGET_TGT_STMT_IMPL_H

#include "tgt_stmt.h"
//#include "tgt_opc_quick.h"

// args is a container of "MCODE_T::arg_t" from comma-separated arguments
//
// Two special caes:
//  1)  if opcode has no arguments (eg "nop"), a single "MODE_NONE"
//      is created for op-code level error messages
//  2)  bitfield arguments (the offset, width portion) are not separated
//      by commas, but are a separate "m68k_arg_t" instance.
namespace kas::tgt
{

using namespace kas::core::opc;

template <typename INSN_T, typename ARG_T>
core::opcode *tgt_stmt<INSN_T, ARG_T>
        ::do_gen_insn(core::opcode::data_t& data)
{
    // get support types from `mcode`
    using mcode_t   = typename insn_t::mcode_t;

    using err_msg_t    = typename mcode_t::err_msg_t;
    using bitset_t     = typename mcode_t::bitset_t;
    using op_size_t    = typename mcode_t::op_size_t;
    using mcode_size_t = typename mcode_t::mcode_size_t;

    // get kas types from opcode
    using core::opcode;
    auto trace  = opcode::trace;
    
    // convenience references 
    auto& insn = *insn_p;
    auto& fixed = data.fixed;

    // XXX ??? why
    trace = &std::cout;

    // generate an "error" opcode if appropriate
    auto make_error = [&fixed=fixed, &trace]
            (parser::tagged_msg const& msg) -> opcode *
        {
            if (trace)
                *trace << "ERR: " << msg.msg << std::endl;
            
            fixed.diag = kas::parser::kas_diag::error(msg).ref();
            return {};
        };

    // copy "opcode" position from first (possibly dummy arg)
    parser::kas_position_tagged opc_pos = args.front();
    
    // if first is dummy, clear args
    if (args.front().is_missing())
        args.clear();

    // print name/args
    if (trace)
    {
        *trace << "tgt_stmt::eval: " << insn.name << " [" << insn.mcodes.size() << " opcodes]";
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
    bitset_t ok;
    mcode_t const* matching_mcode_p {};
    bool multiple_matches = false;
    const char *first_msg{};

    // loop thru mcodes, recording first error & recording all matches
    int i = 0; 
    for (auto mcode_p : insn.mcodes)
    {
        if (trace)
            *trace << "validating: " << +i << ": ";

        auto diag = mcode_p->validate_args(args, trace);
        
        if (trace)
            *trace << (diag ? diag : " = OK") << std::endl;

        if (!diag)
        {
            // match found. record in OK
            // also record iff first matching
            ok.set(i);
            if (!matching_mcode_p)
                matching_mcode_p = mcode_p;
            else
                multiple_matches = true;
        }
        
        // diag: record first to match
        else if (!first_msg)
            first_msg = diag;
        
        ++i;        // next
    }
    
    if (trace)
        *trace << "result: " << ok.count() << " OK" << std::endl;

    // no match: Invalid arguments
    if (!matching_mcode_p)
        return make_error({err_msg_t::ERR_invalid, opc_pos});

    // multiple matches means no match
    if (multiple_matches)
        matching_mcode_p = {};

#if 0
    // logic: here at least one arg matches. 
    // 1) if constant `args`, emit binary code
    // 2) if single match, use format for selected opcode
    // 3) otherwise, use opcode for "list"

    if (args_are_const)
    {
        op_size_t insn_size;
        expression::expr_fits fits;

        // all const args: can select best opcode & calculate size
        if (!matching_mcode_p)
        {
            matching_mcode_p = insn.eval(ok, args, insn_size, fits, trace);
        }

        // single opcode matched: calculate size
        else
        {
            insn_size = matching_mcode_p->base_size();
            for (auto& arg : args)
                insn_size += arg.size(fits);
        }

        if (trace)
            *trace << "size = " << insn_size << std::endl;

        // if binary data fits in "fixed" area of opcode, just emit as binary data
        if (insn_size() <= sizeof(fixed))
        {
            static opc::tgt_opc_quick<mcode_size_t> opc_quick;

            if (opc_quick.proc_args(data, *matching_mcode_p, args, insn_size()))
                return &opc_quick;
        }
    }
#endif
    
    // if `insn` not resolved to single `mcode`, use list
    if (!matching_mcode_p)
        matching_mcode_p = insn_t::list_mcode_p;

#if 0
    return matching_mcode_p->gen_insn(
                  insn
                , ok
                , matching_mcode_p
                , std::move(args)
                
                // and opcode data
                , data
                );
    return op;
#else
    return nullptr;
#endif
}
}

#endif
