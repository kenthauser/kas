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
        ::gen_insn(core::opcode::data_t& data)
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
    trace = nullptr;
    
    // convenience references 
    auto& insn = *insn_p;
    auto& fixed = data.fixed;
    
    
    // print name/args
    if (trace)
    {
        *trace << "tgt_stmt::eval: " << insn.name << " [" << insn.mcodes.size() << " opcodes]";
        for (auto& arg : args)
            *trace << ", " << arg;
        *trace << std::endl;
    }


    // validate args as appropriate for target
    // also note if all args are "const" (ie: just regs & literals)
    bool args_are_const = true;
    if (auto diag = insn.validate_args(args, args_are_const, trace))
    {
        data.fixed.diag = diag;
        return {};
    }

    // select mcode matching args (normally only a single mcode))
    bitset_t ok;
    mcode_t const* matching_mcode_p {};
    bool multiple_matches = false;
    const char *err_msg{};
    int         err_index;

    // loop thru mcodes, recording first error & recording all matches
    int i = 0; 
    for (auto mcode_p : insn.mcodes)
    {
//#define TRACE_STMT_MCODE
#ifdef TRACE_STMT_MCODE
        if (trace)
        {
            *trace << std::dec << std::endl;;
            *trace << "mcode: " << +mcode_p->index      << " ";
            *trace << "defn: "  << +mcode_p->defn_index << " ";
            mcode_p->defn().print(*trace);
        }
#endif
#undef TRACE_STMT_MCODE
        if (trace)
            *trace << "validating: " << +i << ": ";

        auto result = mcode_p->validate_args(args, trace);
        auto diag = result.first;
        
        if (trace)
        {
            if (diag)
                *trace << " -> " << diag << std::endl;
            else
                *trace << " = OK" << std::endl;
        }

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
        
        // diag: record best error message
        else if (!err_msg || result.second > err_index)
        {
            err_msg   = diag;
            err_index = result.second;
        }
        
        ++i;        // next
    }
    
    if (trace)
        *trace << "result: " << ok.count() << " OK" << std::endl;

    // no match: generate message
    if (!matching_mcode_p)
    {
        if (err_index >= args.size())
            err_index  = args.size() - 1;
        data.fixed.diag = parser::kas_diag::error(err_msg, args[err_index]).ref();
        return {};
    }

    // multiple matches means no match
    if (multiple_matches)
        matching_mcode_p = {};

    // no longer need dummy "arg" for error messages.
    if (args.front().is_missing())
        args.clear();


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

#if 1
    return matching_mcode_p->fmt().get_opc().gen_insn(
                  insn
                , ok
                , matching_mcode_p
                , std::move(args)
                
                // and opcode data
                , data
                );
#else
    return nullptr;
#endif
}

// test fixure routine to display statement name
template <typename INSN_T, typename ARG_T>
std::string tgt_stmt<INSN_T, ARG_T>::name() const
{
    using BASE_NAME = typename INSN_T::mcode_t::BASE_NAME;
    
    auto name_prefix = kas::str_cat<BASE_NAME, KAS_STRING(":")>::value;
    return name_prefix + insn_p->name;
}
}

#endif
